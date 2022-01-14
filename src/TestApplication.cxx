#include "sys.h"
#include "TestApplication.h"
#include "SampleParameters.h"
#include "TimerData.h"
#include "vulkan/FrameResourcesData.h"
#include "vulkan/VertexData.h"
#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDeviceFeatures.h"
#include "vulkan/infos/DeviceCreateInfo.h"
#include "vulkan/rendergraph/Attachment.h"
#include "vulkan/rendergraph/RenderPass.h"
#include "vulkan/Pipeline.h"
#include "vk_utils/get_image_data.h"
#include "debug.h"
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif
#include "shaderbuilder/ShaderModule.h"

#include "vulkan/ClearValue.h"
#include "vulkan/ImageKind.h"

using namespace linuxviewer;

class Window : public task::SynchronousWindow
{
 public:
  using task::SynchronousWindow::SynchronousWindow;

 private:
  // Additional image (view) kind.
  static vulkan::ImageKind const s_vector_image_kind;
  static vulkan::ImageViewKind const s_vector_image_view_kind;

  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  RenderPass imgui_pass{this, "imgui_pass"};       // FIXME: put this in the base class?
  Attachment      depth{this, "depth",    s_depth_image_view_kind};
  Attachment   position{this, "position", s_vector_image_view_kind};
  Attachment     normal{this, "normal",   s_vector_image_view_kind};
  Attachment     albedo{this, "albedo",   s_color_image_view_kind};

  vk::UniquePipeline m_graphics_pipeline;
  vulkan::BufferParameters m_vertex_buffer;
  vulkan::BufferParameters m_instance_buffer;
  vulkan::DescriptorSetParameters m_descriptor_set;
  vulkan::ImageParameters m_background_texture;
  vulkan::ImageParameters m_texture;
  vk::UniquePipelineLayout m_pipeline_layout;
  lv_utils::TimerData m_timer;

  SampleParameters Parameters;
  int m_frame_count = 0;

 private:
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 10 frames per second.
    return threadpool::Interval<50, std::chrono::milliseconds>{};
  }

  vulkan::FrameResourceIndex number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{5};
  }

  void set_default_clear_values(vulkan::ClearValue& color, vulkan::ClearValue& depth_stencil) override
  {
    // Use red as default clear color for this window.
//    color = { 1.f, 0.f, 0.f, 1.f };
  }

  void PerformHardcoreCalculations(int duration) const
  {
    auto start_time = std::chrono::high_resolution_clock::now();
    long long calculations_time = 0;
    float m = 300.5678;

    do
    {
      asm volatile ("" : "+r" (m));
      float _sin = std::sin(std::cos(m));
      float _pow = std::pow(m, _sin);
      float _cos = std::cos(std::sin(_pow));
      asm volatile ("" :: "r" (_cos));

      calculations_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }
    while (calculations_time < 1000 * duration);
  }

  void draw_frame() override
  {
    DoutEntering(dc::vkframe, "Window::draw_frame() [frame:" << m_frame_count << "; " << this << "; " << (is_slow() ? "SlowWindow" : "Window") << "]");

    // Skip the first frame.
    if (++m_frame_count == 1)
      return;

    m_current_frame.m_resource_count = Parameters.m_frame_resources_count;  // Slider value.
    Dout(dc::vkframe, "m_current_frame.m_resource_count = " << m_current_frame.m_resource_count);
    auto frame_begin_time = std::chrono::high_resolution_clock::now();

    m_timer.update();
#if 0
  Gui.StartFrame( Timer, MouseState );
  PrepareGUIFrame();
#endif

    // Start frame - calculate times and prepare GUI.
    start_frame();

    // Acquire swapchain image.
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    // Draw scene/prepare scene's command buffers.
    {
      auto frame_generation_begin_time = std::chrono::high_resolution_clock::now();

      // Perform calculation influencing current frame.
      PerformHardcoreCalculations(Parameters.PreSubmitCpuWorkTime);

      // Draw sample-specific data - includes command buffer submission!!
      DrawSample();

      // Perform calculations influencing rendering of a next frame.
      PerformHardcoreCalculations(Parameters.PostSubmitCpuWorkTime);

      auto frame_generation_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_generation_begin_time);
      float float_frame_generation_time = static_cast<float>(frame_generation_time.count() * 0.001f);
      Parameters.FrameGenerationTime = Parameters.FrameGenerationTime * 0.99f + float_frame_generation_time * 0.01f;
    }

    // Draw GUI and present swapchain image.
    finish_frame();

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    float float_frame_time = static_cast<float>(total_frame_time.count() * 0.001f);
    Parameters.TotalFrameTime = Parameters.TotalFrameTime * 0.99f + float_frame_time * 0.01f;

    Dout(dc::vkframe, "Leaving Window::draw_frame with total_frame_time = " << total_frame_time);
  }

  void DrawSample()
  {
    DoutEntering(dc::vkframe, "Window::DrawSample() [" << this << "]");
    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;

    // Record the command buffer of main_pass.
    auto clear_values = main_pass.clear_values();

    std::array<vk::ImageView, 2> attachments = {
      *m_current_frame.m_frame_resources->m_image_parameters[depth].m_image_view,
      swapchain().vh_current_image_view()
    };

#ifdef CWDEBUG
    Dout(dc::vkframe, "m_swapchain.m_current_index = " << swapchain().current_index());
    std::array<vk::Image, 2> attachment_images = {
      swapchain().images()[swapchain().current_index()],
      *m_current_frame.m_frame_resources->m_image_parameters[depth].m_image
    };
    Dout(dc::vkframe, "Attachments: ");
    for (int i = 0; i < 2; ++i)
    {
      Dout(dc::vkframe, "  image_view: " << attachments[i] << ", image: " << attachment_images[i]);
    }
#endif

    auto swapchain_extent = swapchain().extent();
    vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> render_pass_begin_info_chain(
      {
        .renderPass = main_pass.vh_render_pass(),
        .framebuffer = main_pass.vh_framebuffer(),
        .renderArea = {
          .offset = {},
          .extent = swapchain_extent
        },
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data()
      },
      {
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data()
      }
    );
    vk::RenderPassBeginInfo& render_pass_begin_info = render_pass_begin_info_chain.get<vk::RenderPassBeginInfo>();

    //======================================================
    // temporary imgui renderpass stuff.
    auto imgui_clear_values = imgui_pass.clear_values();
    std::array<vk::ImageView, 1> imgui_attachments = {
      swapchain().vh_current_image_view()
    };
    vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> imgui_render_pass_begin_info_chain(
      {
        .renderPass = imgui_pass.vh_render_pass(),
        .framebuffer = imgui_pass.vh_framebuffer(),
        .renderArea = {
          .offset = {},
          .extent = swapchain_extent
        },
        .clearValueCount = static_cast<uint32_t>(imgui_clear_values.size()),
        .pClearValues = imgui_clear_values.data()
      },
      {
        .attachmentCount = imgui_attachments.size(),
        .pAttachments = imgui_attachments.data()
      }
    );
    vk::RenderPassBeginInfo& imgui_render_pass_begin_info = imgui_render_pass_begin_info_chain.get<vk::RenderPassBeginInfo>();
    //======================================================

    vk::Viewport viewport{
      .x = 0,
      .y = 0,
      .width = static_cast<float>(swapchain_extent.width),
      .height = static_cast<float>(swapchain_extent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };

    vk::Rect2D scissor{
      .offset = vk::Offset2D(),
      .extent = swapchain_extent
    };

    float scaling_factor = static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height);

    m_logical_device->reset_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed });
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_command_buffer(command_pool_w);

      Dout(dc::vkframe, "Start recording command buffer.");
      command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
      command_buffer_w->beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
      command_buffer_w->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
      command_buffer_w->setViewport(0, { viewport });
      command_buffer_w->setScissor(0, { scissor });
      command_buffer_w->bindVertexBuffers(0, { *m_vertex_buffer.m_buffer, *m_instance_buffer.m_buffer }, { 0, 0 });
      command_buffer_w->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { *m_descriptor_set.m_handle }, {});
      command_buffer_w->pushConstants(*m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ), &scaling_factor);
      command_buffer_w->draw(6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, Parameters.ObjectsCount, 0, 0);
      command_buffer_w->endRenderPass();
      command_buffer_w->beginRenderPass(imgui_render_pass_begin_info, vk::SubpassContents::eInline);
      command_buffer_w->endRenderPass();
      command_buffer_w->end();
      Dout(dc::vkframe, "End recording command buffer.");

      vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = swapchain().vhp_current_image_available_semaphore(),
        .pWaitDstStageMask = &wait_dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffer_w,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = swapchain().vhp_current_rendering_finished_semaphore()
      };

      Dout(dc::vkframe, "Submitting command buffer: submit({" << submit_info << "}, " << *frame_resources->m_command_buffers_completed << ")");
      presentation_surface().vh_graphics_queue().submit( { submit_info }, *frame_resources->m_command_buffers_completed );
    } // Unlock command pool.

    Dout(dc::vkframe, "Leaving Window::DrawSample.");
  }

  void create_vertex_buffers() override
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers() [" << this << "]");

    // 3D model
    std::vector<vulkan::VertexData> vertex_data;

    float const size = 0.12f;
    float step       = 2.0f * size / SampleParameters::s_quad_tessellation;
    for (int x = 0; x < SampleParameters::s_quad_tessellation; ++x)
    {
      for (int y = 0; y < SampleParameters::s_quad_tessellation; ++y)
      {
        float pos_x = -size + x * step;
        float pos_y = -size + y * step;

        vertex_data.push_back({{pos_x, pos_y, 0.0f, 1.0f}, {static_cast<float>(x) / (SampleParameters::s_quad_tessellation), static_cast<float>(y) / (SampleParameters::s_quad_tessellation)}});
        vertex_data.push_back(
            {{pos_x, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x) / (SampleParameters::s_quad_tessellation), static_cast<float>(y + 1) / (SampleParameters::s_quad_tessellation)}});
        vertex_data.push_back(
            {{pos_x + step, pos_y, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (SampleParameters::s_quad_tessellation), static_cast<float>(y) / (SampleParameters::s_quad_tessellation)}});
        vertex_data.push_back(
            {{pos_x + step, pos_y, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (SampleParameters::s_quad_tessellation), static_cast<float>(y) / (SampleParameters::s_quad_tessellation)}});
        vertex_data.push_back(
            {{pos_x, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x) / (SampleParameters::s_quad_tessellation), static_cast<float>(y + 1) / (SampleParameters::s_quad_tessellation)}});
        vertex_data.push_back(
            {{pos_x + step, pos_y + step, 0.0f, 1.0f}, {static_cast<float>(x + 1) / (SampleParameters::s_quad_tessellation), static_cast<float>(y + 1) / (SampleParameters::s_quad_tessellation)}});
      }
    }

    m_vertex_buffer = logical_device().create_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(vulkan::VertexData),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_vertex_buffer")));
    copy_data_to_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(vulkan::VertexData), vertex_data.data(), *m_vertex_buffer.m_buffer, 0, vk::AccessFlags(0),
        vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);

    // Per instance data (position offsets and distance)
    std::vector<float> instance_data(SampleParameters::s_max_objects_count * 4);
    for (size_t i = 0; i < instance_data.size(); i += 4)
    {
      instance_data[i]     = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
      instance_data[i + 1] = static_cast<float>(std::rand() % 513) / 256.0f - 1.0f;
      instance_data[i + 2] = static_cast<float>(std::rand() % 513) / 512.0f;
      instance_data[i + 3] = 0.0f;
    }

    m_instance_buffer = logical_device().create_buffer(static_cast<uint32_t>(instance_data.size()) * sizeof(float),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_instance_buffer")));
    copy_data_to_buffer(static_cast<uint32_t>(instance_data.size()) * sizeof(float), instance_data.data(), *m_instance_buffer.m_buffer, 0, vk::AccessFlags(0),
        vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);
  }

  void create_render_passes() override
  {
    DoutEntering(dc::vulkan, "Window::create_render_passes() [" << this << "]");

#ifdef CWDEBUG
//    vulkan::rendergraph::RenderGraph::testsuite();
#endif

    // This must be a reference.
    auto& output = swapchain().presentation_attachment();

    // Change the clear values.
//    depth.set_clear_value({1.f, 0xffff0000});
//    swapchain().set_clear_value_presentation_attachment({0.f, 1.f, 1.f, 1.f});

    // Define the render graph.
    m_render_graph = main_pass[~depth]->stores(~output) >> imgui_pass->stores(output);

    // Generate everything.
    m_render_graph.generate(this);

#if 0
    // Render pass - from present_src to color_attachment.
    {
      utils::Vector<vk_defaults::AttachmentDescription, vulkan::rendergraph::pAttachmentsIndex> attachment_descriptions = {
        vk::AttachmentDescription{
          .format = s_default_depth_format,
          .loadOp = vk::AttachmentLoadOp::eClear,
          .storeOp = vk::AttachmentStoreOp::eStore,
          .initialLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
          .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        },
        vk::AttachmentDescription{
          .format = swapchain().image_kind()->format,
          .loadOp = vk::AttachmentLoadOp::eClear,
          .storeOp = vk::AttachmentStoreOp::eStore,
          .initialLayout = vk::ImageLayout::ePresentSrcKHR,
          .finalLayout = vk::ImageLayout::ePresentSrcKHR //vk::ImageLayout::eColorAttachmentOptimal
        }
      };
    }
#endif

#if 0
    // Post-render pass - from color_attachment to present_src.
    {
      std::vector<vulkan::RenderPassAttachmentData> attachment_descriptions = {
        {
          .m_format = swapchain().image_kind()->format,
          .m_load_op = vk::AttachmentLoadOp::eLoad,
          .m_store_op = vk::AttachmentStoreOp::eStore,
          .m_initial_layout = vk::ImageLayout::eColorAttachmentOptimal,
          .m_final_layout = vk::ImageLayout::ePresentSrcKHR
        },
        {
          .m_format = s_default_depth_format,
          .m_load_op = vk::AttachmentLoadOp::eDontCare,
          .m_store_op = vk::AttachmentStoreOp::eDontCare,
          .m_initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
          .m_final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
      };
      m_post_render_pass = logical_device().create_render_pass(attachment_descriptions, subpass_descriptions, dependencies
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_post_render_pass")));
    }
#endif
  }

  void create_descriptor_set() override
  {
    DoutEntering(dc::vulkan, "Window::create_descriptor_set() [" << this << "]");

    std::vector<vk::DescriptorSetLayoutBinding> layout_bindings = {
      {
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      },
      {
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr
      }
    };
    std::vector<vk::DescriptorPoolSize> pool_sizes = {
    {
      .type = vk::DescriptorType::eCombinedImageSampler,
      .descriptorCount = 2
    }};

    m_descriptor_set = logical_device().create_descriptor_resources(layout_bindings, pool_sizes
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_descriptor_set")));
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    // Background texture.
    {
      int width = 0, height = 0, data_size = 0;
      std::vector<std::byte> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/background.png", 4, &width, &height, nullptr, &data_size);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const background_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const background_image_view_kind(background_image_kind, {});

        m_background_texture =
          m_logical_device->create_image(width, height, background_image_kind, background_image_view_kind, vk::MemoryPropertyFlagBits::eDeviceLocal
              COMMA_CWDEBUG_ONLY(debug_name_prefix("m_background_texture")));

        m_background_texture.m_sampler =
          m_logical_device->create_sampler(
              vk::SamplerMipmapMode::eNearest,
              vk::SamplerAddressMode::eClampToEdge,
              VK_FALSE
              COMMA_CWDEBUG_ONLY(debug_name_prefix("m_background_texture.m_sampler")));
      }
      // Copy data.
      {
        vk_defaults::ImageSubresourceRange const image_subresource_range;
        copy_data_to_image(data_size, texture_data.data(), *m_background_texture.m_image, width, height, image_subresource_range, vk::ImageLayout::eUndefined,
            vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eFragmentShader);
      }
      // Update descriptor set.
      {
        std::vector<vk::DescriptorImageInfo> image_infos = {
          {
            .sampler = *m_background_texture.m_sampler,
            .imageView = *m_background_texture.m_image_view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
          }
        };
        m_logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos);
      }
    }

    // Sample texture.
    {
      int width = 0, height = 0, data_size = 0;
      std::vector<std::byte> texture_data = vk_utils::get_image_data(m_application->resources_path() / "textures/frame_resources.png", 4, &width, &height, nullptr, &data_size);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const sample_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

        m_texture = m_logical_device->create_image(width, height, sample_image_kind, sample_image_view_kind, vk::MemoryPropertyFlagBits::eDeviceLocal
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture")));
        m_texture.m_sampler = m_logical_device->create_sampler(vk::SamplerMipmapMode::eNearest, vk::SamplerAddressMode::eClampToEdge, VK_FALSE
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture.m_sampler")));
      }
      // Copy data.
      {
        vk_defaults::ImageSubresourceRange const image_subresource_range;
        copy_data_to_image(data_size, texture_data.data(), *m_texture.m_image, width, height, image_subresource_range, vk::ImageLayout::eUndefined,
            vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe, vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead,
            vk::PipelineStageFlagBits::eFragmentShader);
      }
      // Update descriptor set.
      {
        std::vector<vk::DescriptorImageInfo> image_infos = {
          {
            .sampler = *m_texture.m_sampler,
            .imageView = *m_texture.m_image_view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
          }
        };
        m_logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 1, 0, image_infos);
      }
    }
  }

  void create_pipeline_layout() override
  {
    DoutEntering(dc::vulkan, "Window::create_pipeline_layout() [" << this << "]");

    vk::PushConstantRange push_constant_ranges{
      .stageFlags = vk::ShaderStageFlagBits::eVertex,
      .offset = 0,
      .size = 4
    };
    m_pipeline_layout = m_logical_device->create_pipeline_layout({ *m_descriptor_set.m_layout }, { push_constant_ranges }
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline_layout")));
  }

  static constexpr std::string_view intel_vert_glsl = R"glsl(
#version 450

layout(location = 0) in vec4 i_Position;
layout(location = 1) in vec2 i_Texcoord;
layout(location = 2) in vec4 i_PerInstanceData;

layout( push_constant ) uniform Scaling {
  float AspectScale;
} PushConstant;

out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec2 v_Texcoord;
layout(location = 1) out float v_Distance;

void main()
{
  v_Texcoord = i_Texcoord;
  v_Distance = 1.0 - i_PerInstanceData.z;       // Darken with distance

  vec4 position = i_Position;
  position.y *= PushConstant.AspectScale;      // Adjust to screen aspect ration
  position.xy *= pow( v_Distance, 0.5 );       // Scale with distance
  gl_Position = position + i_PerInstanceData;
}
)glsl";

  static constexpr std::string_view intel_frag_glsl = R"glsl(
#version 450

layout(set=0, binding=0) uniform sampler2D u_BackgroundTexture;
layout(set=0, binding=1) uniform sampler2D u_BenchmarkTexture;

layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) in float v_Distance;

layout(location = 0) out vec4 o_Color;

void main() {
  vec4 background_image = texture( u_BackgroundTexture, v_Texcoord );
  vec4 benchmark_image = texture( u_BenchmarkTexture, v_Texcoord );
  o_Color = v_Distance * mix( background_image, benchmark_image, benchmark_image.a );
}
)glsl";

  void create_graphics_pipeline() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipeline() [" << this << "]");

    vulkan::Pipeline pipeline(this);

    {
      using namespace vulkan::shaderbuilder;

      ShaderCompiler compiler;
      ShaderCompilerOptions options;

      ShaderModule shader_vert(vk::ShaderStageFlagBits::eVertex);
      shader_vert.set_name("intel.vert.glsl").load(intel_vert_glsl).compile(compiler, options);
      pipeline.add(shader_vert COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::pipeline")));

      ShaderModule shader_frag(vk::ShaderStageFlagBits::eFragment);
      shader_frag.set_name("intel.frag.glsl").load(intel_frag_glsl).compile(compiler, options);
      pipeline.add(shader_frag COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::pipeline")));
    }

    std::vector<vk::VertexInputBindingDescription> vertex_binding_description = {
      {
        .binding = 0,
        .stride = sizeof(vulkan::VertexData),
        .inputRate = vk::VertexInputRate::eVertex
      },
      {
        .binding = 1,
        .stride = 4 * sizeof(float),
        .inputRate = vk::VertexInputRate::eInstance
      }
    };
    std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
      {
        .location = 0,
        .binding = vertex_binding_description[0].binding,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = 0
      },
      {
        .location = 1,
        .binding = vertex_binding_description[0].binding,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(vulkan::VertexData, m_texture_coordinates)
      },
      {
        .location = 2,
        .binding = vertex_binding_description[1].binding,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = 0
      }
    };

    //=========================================================================
    // Vertex input.

    vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
      .flags = vk::PipelineVertexInputStateCreateFlags(0),
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size()),
      .pVertexBindingDescriptions = vertex_binding_description.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
      .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
      .flags = vk::PipelineInputAssemblyStateCreateFlags(0),
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = VK_FALSE
    };

    vk::PipelineViewportStateCreateInfo viewport_state_create_info{
      .flags = vk::PipelineViewportStateCreateFlags(0),
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr
    };
    vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
      .flags = vk::PipelineRasterizationStateCreateFlags(0),
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0f,
      .depthBiasClamp = 0.0f,
      .depthBiasSlopeFactor = 0.0f,
      .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisample_state_create_info{
      .flags = vk::PipelineMultisampleStateCreateFlags(0),
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
      .flags = vk::PipelineDepthStencilStateCreateFlags(0),
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = vk::CompareOp::eLessOrEqual
    };

    vk::PipelineColorBlendAttachmentState color_blend_attachment_state{
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
      .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
      .colorBlendOp = vk::BlendOp::eAdd,
      .srcAlphaBlendFactor = vk::BlendFactor::eOne,
      .dstAlphaBlendFactor = vk::BlendFactor::eZero,
      .alphaBlendOp = vk::BlendOp::eAdd,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    };

    vk::PipelineColorBlendStateCreateInfo color_blend_state_create_info{
      .flags = vk::PipelineColorBlendStateCreateFlags(0),
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment_state
    };

    std::vector<vk::DynamicState> dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
      .flags = vk::PipelineDynamicStateCreateFlags(0),
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()
    };

    auto const& shader_stage_create_infos = pipeline.shader_stage_create_infos();

    vk::GraphicsPipelineCreateInfo pipeline_create_info{
      .flags = vk::PipelineCreateFlags(0),
      .stageCount = static_cast<uint32_t>(shader_stage_create_infos.size()),
      .pStages = shader_stage_create_infos.data(),
      .pVertexInputState = &vertex_input_state_create_info,
      .pInputAssemblyState = &input_assembly_state_create_info,
      .pTessellationState = nullptr,
      .pViewportState = &viewport_state_create_info,
      .pRasterizationState = &rasterization_state_create_info,
      .pMultisampleState = &multisample_state_create_info,
      .pDepthStencilState = &depth_stencil_state_create_info,
      .pColorBlendState = &color_blend_state_create_info,
      .pDynamicState = &dynamic_state_create_info,
      .layout = *m_pipeline_layout,
      .renderPass = main_pass.vh_render_pass(),
      .subpass = 0,
      .basePipelineHandle = vk::Pipeline{},
      .basePipelineIndex = -1
    };

    m_graphics_pipeline = logical_device().create_graphics_pipeline(vk::PipelineCache{}, pipeline_create_info
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_graphics_pipeline")));
  }
};

vulkan::ImageKind const Window::s_vector_image_kind{{ .format = vk::Format::eR16G16B16A16Sfloat }};
vulkan::ImageViewKind const Window::s_vector_image_view_kind{s_vector_image_kind, {}};

class WindowEvents : public vulkan::WindowEvents
{
 private:
  void MouseMove(int x, int y) override
  {
    DoutEntering(dc::notice, "WindowEvents::MouseMove(" << x << ", " << y << ")");
  }

  void MouseClick(size_t button, bool pressed) override
  {
    DoutEntering(dc::notice, "WindowEvents::MouseClick(" << button << ", " << pressed << ")");
  }

  void ResetMouse() override
  {
    DoutEntering(dc::notice, "WindowEvents::ResetMouse()");
  }
};

class SlowWindow : public Window
{
 public:
  using Window::Window;

 private:
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 1 frames per second.
    return threadpool::Interval<1000, std::chrono::milliseconds>{};
  }

  bool is_slow() const override
  {
    return true;
  }
};

class LogicalDevice : public vulkan::LogicalDevice
{
 public:
  // Every time create_root_window is called a cookie must be passed.
  // This cookie will be passed back to the virtual function ... when
  // querying what presentation queue family to use for that window (and
  // related windows).
  static constexpr int root_window_cookie1 = 1;
  static constexpr int root_window_cookie2 = 2;

  LogicalDevice()
  {
    DoutEntering(dc::notice, "LogicalDevice::LogicalDevice() [" << this << "]");
  }

  ~LogicalDevice() override
  {
    DoutEntering(dc::notice, "LogicalDevice::~LogicalDevice() [" << this << "]");
  }

  void prepare_physical_device_features(vulkan::PhysicalDeviceFeatures& physical_device_features) const override
  {
    // Use the setters from vk::PhysicalDeviceFeatures.
    physical_device_features.setDepthClamp(true);
    // FIXME: Allow adding features to the pNext chain of vk::PhysicalDeviceFeatures2.
  }

  void prepare_logical_device(vulkan::DeviceCreateInfo& device_create_info) const override
  {
    using vulkan::QueueFlagBits;

    device_create_info
    // {0}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eGraphics,
        .max_number_of_queues = 13,
        .priority = 1.0})
    // {1}
//    .combineQueueRequest({
    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 8,                      // Only used when it can not be combined.
        .priority = 0.8,                                // Only used when it can not be combined.
        .windows = root_window_cookie1})                // This may only be used for window1.
    // {2}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 2,
        .priority = 0.2,
        .windows = root_window_cookie2})
#ifdef CWDEBUG
    .setDebugName("LogicalDevice");
#endif
    ;
  }
};

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  try
  {
    // Create the application object.
    TestApplication application;

    // Initialize application using the virtual functions of TestApplication.
    application.initialize(argc, argv);

    // Create a window.
    auto root_window1 = application.create_root_window<WindowEvents, Window>({1000, 800}, LogicalDevice::root_window_cookie1);

    // Create a logical device that supports presenting to root_window1.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));

    // Assume logical_device also supports presenting on root_window2.
//    application.create_root_window<WindowEvents, SlowWindow>({400, 400}, LogicalDevice::root_window_cookie1, *logical_device, "Second window");

    // Run the application.
    application.run();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, "\e[31m" << error << ", caught in TestApplication.cxx\e[0m");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "\e[31mstd::exception: " << exception.what() << " caught in TestApplication.cxx\e[0m");
  }
#endif

  Dout(dc::notice, "Leaving main()");
}
