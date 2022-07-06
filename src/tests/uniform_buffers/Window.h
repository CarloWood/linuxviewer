#pragma once

#include "UniformBuffersTest.h"
#include "vulkan/SynchronousWindow.h"
#include "queues/CopyDataToImage.h"
#include "vk_utils/ImageData.h"
#include <imgui.h>
#include "debug.h"
#include "tracy/CwTracy.h"
#ifdef TRACY_ENABLE
#include "tracy/SourceLocationDataIterator.h"
#endif

#define ENABLE_IMGUI 1

class Window : public task::SynchronousWindow
{
 private:
  using Directory = vulkan::Directory;

 public:
  using task::SynchronousWindow::SynchronousWindow;

 private:
  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth", s_depth_image_view_kind};

  vulkan::Texture m_texture;

  vk::UniquePipelineLayout m_pipeline_layout;
  vk::Pipeline m_vh_graphics_pipeline;

  imgui::StatsWindow m_imgui_stats_window;
  int m_frame_count = 0;

 private:
  void create_render_graph() override
  {
    DoutEntering(dc::vulkan, "Window::create_render_graph() [" << this << "]");

    // This must be a reference.
    auto& output = swapchain().presentation_attachment();

    // Define the render graph.
    m_render_graph = main_pass[~depth]->stores(~output)
#if ENABLE_IMGUI
      >> imgui_pass->stores(output)
#endif
      ;

    // Generate everything.
    m_render_graph.generate(this);
  }

  vulkan::SwapchainIndex max_number_of_swapchain_images() const override
  {
    return vulkan::SwapchainIndex{4};
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

    m_descriptor_set = logical_device()->create_descriptor_resources(layout_bindings, pool_sizes
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_descriptor_set")));
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    // Sample texture.
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / "textures/vort3_128x128.png", 4);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const sample_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

        m_texture = vulkan::Texture(m_logical_device,
            texture_data.extent(), sample_image_view_kind,
            { .mipmapMode = vk::SamplerMipmapMode::eNearest,
              .anisotropyEnable = VK_FALSE },
            graphics_settings(),
            { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture")));

        auto copy_data_to_image = statefultask::create<task::CopyDataToImage>(m_logical_device, texture_data.size(),
            m_texture.m_vh_image, texture_data.extent(), vk_defaults::ImageSubresourceRange{},
            vk::ImageLayout::eUndefined, vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
            COMMA_CWDEBUG_ONLY(true));

        copy_data_to_image->set_resource_owner(this);   // Wait for this task to finish before destroying this window, because this window owns the texture (m_texture).
        copy_data_to_image->set_data_feeder(std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)));
        copy_data_to_image->run(vulkan::Application::instance().low_priority_queue());
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
        m_logical_device->update_descriptor_set(*m_descriptor_set.m_handle, vk::DescriptorType::eCombinedImageSampler, 0, 0, image_infos);
      }
    }
  }

  void create_pipeline_layout() override
  {
    DoutEntering(dc::vulkan, "Window::create_pipeline_layout() [" << this << "]");

    m_pipeline_layout = m_logical_device->create_pipeline_layout({ *m_descriptor_set.m_layout }, { }
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline_layout")));
  }

  static constexpr std::string_view uniform_buffer_controlled_triangle_vert_glsl = R"glsl(
#version 450

layout(location = 0) out vec2 v_Texcoord;

vec2 positions[3] = vec2[](
    vec2(0.0, -1.0),
    vec2(-1.0, 1.0),
    vec2(1.0, 1.0)
);

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  v_Texcoord = 0.5 * (positions[gl_VertexIndex] + vec2(1.0, 1.0));
}
)glsl";

  static constexpr std::string_view uniform_buffer_controlled_triangle_frag_glsl = R"glsl(
#version 450
layout(set=0, binding=0) uniform sampler2D u_Vort3Texture;

layout(location = 0) in vec2 v_Texcoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 vort3_image = texture(u_Vort3Texture, v_Texcoord);
  outColor = vort3_image;
}
)glsl";

  class UniformBuffersTestPipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
    std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    void initialize(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow* owning_window) override
    {
      // Register the vectors that we will fill.
      flat_create_info.add(m_pipeline.shader_stage_create_infos());
      flat_create_info.add(m_pipeline_color_blend_attachment_states);
      flat_create_info.add(m_dynamic_states);

      // Add default color blend.
      m_pipeline_color_blend_attachment_states.push_back(vk_defaults::PipelineColorBlendAttachmentState{});

      // Compile the shaders.
      {
        using namespace vulkan::shaderbuilder;

        ShaderInfo shader_vert(vk::ShaderStageFlagBits::eVertex, "uniform_buffer_controlled_triangle.vert.glsl");
        ShaderInfo shader_frag(vk::ShaderStageFlagBits::eFragment, "uniform_buffer_controlled_triangle.frag.glsl");

        shader_vert.load(uniform_buffer_controlled_triangle_vert_glsl);
        shader_frag.load(uniform_buffer_controlled_triangle_frag_glsl);

        ShaderCompiler compiler;

        m_pipeline.build_shader(owning_window, shader_vert, compiler
            COMMA_CWDEBUG_ONLY({ owning_window, "UniformBuffersTestPipelineCharacteristic::pipeline" }));
        m_pipeline.build_shader(owning_window, shader_frag, compiler
            COMMA_CWDEBUG_ONLY({ owning_window, "UniformBuffersTestPipelineCharacteristic::pipeline" }));
      }

      flat_create_info.m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
    }

   public:
    UniformBuffersTestPipelineCharacteristic() = default;

#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (UniformBuffersTestPipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    auto pipeline_factory = create_pipeline_factory(*m_pipeline_layout, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory.add_characteristic<UniformBuffersTestPipelineCharacteristic>(this);
    pipeline_factory.generate(this);
  }

  void new_pipeline(vulkan::pipeline::Handle pipeline_handle) override
  {
    m_vh_graphics_pipeline = vh_graphics_pipeline(pipeline_handle);
  }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    DoutEntering(dc::vulkan, "Window::get_frame_rate_interval() [" << this << "]");
    // Limit the frame rate of this window to 1000 frames per second.
    return threadpool::Interval<1, std::chrono::milliseconds>{};
  }

  //===========================================================================
  //
  // Frame code (called every frame)
  //
  //===========================================================================

  void render_frame() override
  {
    DoutEntering(dc::vkframe, "Window::render_frame() [frame:" << (m_frame_count + 1) << "; " << this << "; Window]");
    ZoneScopedN("Window::render_frame");

    ++m_frame_count;

    auto frame_begin_time = std::chrono::high_resolution_clock::now();

    // Start frame.
    start_frame();

    // Acquire swapchain image.
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    // Draw scene/prepare scene's command buffers.
    {
      // Draw sample-specific data - includes command buffer submission!!
      draw_frame();
    }

    // Draw GUI and present swapchain image.
    finish_frame();

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    Dout(dc::vkframe, "Leaving Window::render_frame with total_frame_time = " << total_frame_time);
  }

  void draw_frame()
  {
    ZoneScopedN("Window::draw_frame");
    DoutEntering(dc::vkframe, "Window::draw_frame() [" << this << "]");
    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;

    auto swapchain_extent = swapchain().extent();
    main_pass.update_image_views(swapchain(), frame_resources);
#if ENABLE_IMGUI
    imgui_pass.update_image_views(swapchain(), frame_resources);
#endif
#ifdef TRACY_ENABLE
    vulkan::SwapchainIndex const swapchain_index = swapchain().current_index();
#endif

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

    wait_command_buffer_completed();
    m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });

    auto command_buffer = frame_resources->m_command_buffer;
    Dout(dc::vkframe, "Start recording command buffer.");
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass1, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass2, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);

      command_buffer->beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
if (!m_vh_graphics_pipeline)
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_vh_graphics_pipeline);
      command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { *m_descriptor_set.m_handle }, {});
      command_buffer->setViewport(0, { viewport });
      command_buffer->setScissor(0, { scissor });
//      command_buffer->draw(6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, m_sample_parameters.ObjectCount, 0, 0);
      command_buffer->draw(3, 1, 0, 0);
}
      command_buffer->endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#if ENABLE_IMGUI
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass1, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass2, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);
      command_buffer->beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
      m_imgui.render_frame(command_buffer, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
      command_buffer->endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#endif
    command_buffer->end();
    Dout(dc::vkframe, "End recording command buffer.");

    submit(command_buffer);

    Dout(dc::vkframe, "Leaving Window::draw_frame.");
  }

  //===========================================================================
  //
  // ImGui
  //
  //===========================================================================

  UniformBuffersTest const& application() const
  {
    return static_cast<UniformBuffersTest const&>(task::SynchronousWindow::application());
  }

  float m_top_position{};

  void draw_imgui() override final
  {
    ZoneScopedN("Window::draw_imgui");
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_timer);

    //ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f));
    ImGui::Begin(reinterpret_cast<char const*>(application().application_name().c_str()), nullptr, ImGuiWindowFlags_None);
    ImGui::SliderFloat("Top position", &m_top_position, 0.0, 1.0);
    ImGui::End();
  }
};
