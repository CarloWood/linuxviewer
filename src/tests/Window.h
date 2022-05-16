#pragma once

#include "HeavyRectangle.h"
#include "RandomPositions.h"
#include "SampleParameters.h"
#include "FrameResourcesCount.h"
#include "queues/CopyDataToBuffer.h"
#include "vulkan/SynchronousWindow.h"
#include "vk_utils/get_image_data.h"
#include <imgui.h>
#include "debug.h"

#define ENABLE_IMGUI 1

class Window : public task::SynchronousWindow
{
 private:
  using Directory = vulkan::Directory;

 public:
  using task::SynchronousWindow::SynchronousWindow;

 private:
  // Additional image (view) kind.
  static vulkan::ImageKind const s_vector_image_kind;
  static vulkan::ImageViewKind const s_vector_image_view_kind;

  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth",    s_depth_image_view_kind};
  Attachment   position{this, "position", s_vector_image_view_kind};
  Attachment     normal{this, "normal",   s_vector_image_view_kind};
  Attachment     albedo{this, "albedo",   s_color_image_view_kind};

  // Vertex buffers.
  std::vector<vulkan::memory::Buffer> m_vertex_buffers;

  vulkan::Texture m_background_texture;
  vulkan::Texture m_texture;
  vk::UniquePipelineLayout m_pipeline_layout;
  vk::Pipeline m_vh_graphics_pipeline;

  imgui::StatsWindow m_imgui_stats_window;
  SampleParameters m_sample_parameters;
  int m_frame_count = 0;

 private:
  void set_default_clear_values(vulkan::rendergraph::ClearValue& color, vulkan::rendergraph::ClearValue& depth_stencil) override
  {
    DoutEntering(dc::vulkan, "Window::set_default_clear_values() [" << this << "]");
    // Use red as default clear color for this window.
//    color = { 1.f, 0.f, 0.f, 1.f };
  }

  void create_render_graph() override
  {
    DoutEntering(dc::vulkan, "Window::create_render_graph() [" << this << "]");

#ifdef CWDEBUG
//    vulkan::rendergraph::RenderGraph::testsuite();
#endif

    // This must be a reference.
    auto& output = swapchain().presentation_attachment();

    // Change the clear values.
//    depth.set_clear_value({1.f, 0xffff0000});
//    swapchain().set_clear_value_presentation_attachment({0.f, 1.f, 1.f, 1.f});

    // Define the render graph.
    m_render_graph = main_pass[~depth]->stores(~output)
#if ENABLE_IMGUI
      >> imgui_pass->stores(output)
#endif
      ;

    // Generate everything.
    m_render_graph.generate(this);
  }

  vulkan::FrameResourceIndex number_of_frame_resources() const override
  {
    DoutEntering(dc::vulkan, "Window::number_of_frame_resources() [" << this << "]");
    return vulkan::FrameResourceIndex{5};
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

    // Background texture.
    {
      int width = 0, height = 0, data_size = 0;
      std::vector<std::byte> texture_data =
        vk_utils::get_image_data(m_application->path_of(Directory::resources) / "textures/background.png", 4, &width, &height, nullptr, &data_size);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const background_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const background_image_view_kind(background_image_kind, {});

        m_background_texture =
          m_logical_device->create_texture(
              width, height, background_image_view_kind,
              vk::MemoryPropertyFlagBits::eDeviceLocal,
              { .mipmapMode = vk::SamplerMipmapMode::eNearest,
                .anisotropyEnable = VK_FALSE },
              graphics_settings()
              COMMA_CWDEBUG_ONLY(debug_name_prefix("m_background_texture")));
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
      std::vector<std::byte> texture_data =
        vk_utils::get_image_data(m_application->path_of(Directory::resources) / "textures/frame_resources.png", 4, &width, &height, nullptr, &data_size);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const sample_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

        m_texture = m_logical_device->create_texture(
            width, height, sample_image_view_kind,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            { .mipmapMode = vk::SamplerMipmapMode::eNearest,
              .anisotropyEnable = VK_FALSE },
            graphics_settings()
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_texture")));
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
  v_Texcoord = VertexData::m_texture_coordinates;
  v_Distance = 1.0 - InstanceData::m_position.z;        // Darken with distance

  vec4 position = VertexData::m_position;
  position.y *= PushConstant.AspectScale;               // Adjust to screen aspect ration
  position.xy *= pow( v_Distance, 0.5 );                // Scale with distance
  gl_Position = position + InstanceData::m_position;
}
)glsl";

  static constexpr std::string_view intel_frag_glsl = R"glsl(
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

  class FrameResourcesCountPipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
    vulkan::pipeline::Pipeline m_pipeline;
    std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
    std::vector<vk::PipelineColorBlendAttachmentState> const m_pipeline_color_blend_attachment_states{{
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
    }};
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    // Define pipeline objects.
    HeavyRectangle m_heavy_rectangle;           // A rectangle with many vertices.
    RandomPositions m_random_positions;         // Where to put those rectangles.

    void initialize(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow* owning_window) override
    {
      // Register the vectors that we will fill.
      flat_create_info.add(m_vertex_input_binding_descriptions);
      flat_create_info.add(m_vertex_input_attribute_descriptions);
      flat_create_info.add(m_pipeline.shader_stage_create_infos());
      flat_create_info.add(m_pipeline_color_blend_attachment_states);
      flat_create_info.add(m_dynamic_states);

      // Define the pipeline.
      m_pipeline.add_vertex_input_binding(m_heavy_rectangle);
      m_pipeline.add_vertex_input_binding(m_random_positions);

      // Compile the shaders.
      {
        using namespace vulkan::shaderbuilder;

        ShaderInfo shader_vert(vk::ShaderStageFlagBits::eVertex, "intel.vert.glsl");
        ShaderInfo shader_frag(vk::ShaderStageFlagBits::eFragment, "intel.frag.glsl");

        shader_vert.load(intel_vert_glsl);
        shader_frag.load(intel_frag_glsl);

        ShaderCompiler compiler;

        m_pipeline.build_shader(owning_window, shader_vert, compiler
            COMMA_CWDEBUG_ONLY({ owning_window, "FrameResourcesCountPipelineCharacteristic::pipeline" }));
        m_pipeline.build_shader(owning_window, shader_frag, compiler
            COMMA_CWDEBUG_ONLY({ owning_window, "FrameResourcesCountPipelineCharacteristic::pipeline" }));
      }

      m_vertex_input_binding_descriptions = m_pipeline.vertex_binding_descriptions();
      m_vertex_input_attribute_descriptions = m_pipeline.vertex_attribute_descriptions();

      flat_create_info.m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
    }

    // This is called *after* initialize. So m_pipeline is already initialized.
    void synchronous_initialize(task::SynchronousWindow* owning_window) override
    {
      // FIXME: This is a hack. The whole synchronous_initialize should not exist: we should be able to copy vertex buffers asynchronously.
      // Generate vertex buffers.
      static_cast<Window*>(owning_window)->create_vertex_buffers(m_pipeline);
    }

   public:
    FrameResourcesCountPipelineCharacteristic() = default;

#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (FrameResourcesCountPipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    //FIXME: the pipeline layout can vary between different pipelines too; use a vulkan::pipeline::CharacteristicRange for it
    // as well and reuse compatible ones.
    auto pipeline_factory = create_pipeline_factory(*m_pipeline_layout, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory.add_characteristic<FrameResourcesCountPipelineCharacteristic>(this);
    pipeline_factory.generate(this);

    auto pipeline_factory2 = create_pipeline_factory(*m_pipeline_layout, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory2.add_characteristic<FrameResourcesCountPipelineCharacteristic>(this);
    pipeline_factory2.generate(this);
  }

  void new_pipeline(vulkan::pipeline::Handle pipeline_handle) override
  {
    m_vh_graphics_pipeline = vh_graphics_pipeline(pipeline_handle);
  }

  void create_vertex_buffers(vulkan::pipeline::Pipeline const& pipeline)
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers() [" << this << "]");

    for (vulkan::shaderbuilder::VertexShaderInputSetBase* vertex_shader_input_set : pipeline.vertex_shader_input_sets())
    {
      size_t entry_size = vertex_shader_input_set->size();
      int count = vertex_shader_input_set->count();
      size_t buffer_size = count * entry_size;

      m_vertex_buffers.push_back(vulkan::memory::Buffer(logical_device(), buffer_size,
          vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
          0, vk::MemoryPropertyFlagBits::eDeviceLocal
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_vertex_buffers[" + std::to_string(m_vertex_buffers.size()) + "]"))));

      auto copy_data_to_buffer = statefultask::create<task::CopyDataToBuffer>(logical_device(), buffer_size, m_vertex_buffers.back().m_vh_buffer, 0, vk::AccessFlags(0),
          vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput
          COMMA_CWDEBUG_ONLY(true));

      // FIXME: write directly into allocated vulkan buffer?
      std::vector<std::byte>& buf = copy_data_to_buffer->get_buf();
      std::byte const* const end = buf.data() + buffer_size;
      int batch_size;
      for (std::byte* ptr = buf.data(); ptr != end; ptr += batch_size * entry_size)
      {
        batch_size = vertex_shader_input_set->next_batch();
        vertex_shader_input_set->get_input_entry(ptr);
      }
      copy_data_to_buffer->run(vulkan::Application::instance().low_priority_queue());
    }
  }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    DoutEntering(dc::vulkan, "Window::get_frame_rate_interval() [" << this << "]");
    // Limit the frame rate of this window to 20 frames per second.
    return threadpool::Interval<50, std::chrono::milliseconds>{};
  }

  //===========================================================================
  //
  // Frame code (called every frame)
  //
  //===========================================================================

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

    ASSERT(m_sample_parameters.FrameResourcesCount >= 0);
    m_current_frame.m_resource_count = vulkan::FrameResourceIndex{static_cast<size_t>(m_sample_parameters.FrameResourcesCount)};        // Slider value.
    Dout(dc::vkframe, "m_current_frame.m_resource_count = " << m_current_frame.m_resource_count);
    auto frame_begin_time = std::chrono::high_resolution_clock::now();

//    if (m_frame_count == 10)
//      Debug(attach_gdb());

    // Start frame.
    start_frame();

    // Acquire swapchain image.
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    // Draw scene/prepare scene's command buffers.
    {
      auto frame_generation_begin_time = std::chrono::high_resolution_clock::now();

      // Perform calculation influencing current frame.
      PerformHardcoreCalculations(m_sample_parameters.PreSubmitCpuWorkTime);

      // Draw sample-specific data - includes command buffer submission!!
      DrawSample();

      // Perform calculations influencing rendering of a next frame.
      PerformHardcoreCalculations(m_sample_parameters.PostSubmitCpuWorkTime);

      auto frame_generation_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_generation_begin_time);
      float float_frame_generation_time = static_cast<float>(frame_generation_time.count() * 0.001f);
      m_sample_parameters.m_frame_generation_time = m_sample_parameters.m_frame_generation_time * 0.99f + float_frame_generation_time * 0.01f;
    }

    // Draw GUI and present swapchain image.
    finish_frame();

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    float float_frame_time = static_cast<float>(total_frame_time.count() * 0.001f);
    m_sample_parameters.m_total_frame_time = m_sample_parameters.m_total_frame_time * 0.99f + float_frame_time * 0.01f;

    Dout(dc::vkframe, "Leaving Window::draw_frame with total_frame_time = " << total_frame_time);
  }

  void DrawSample()
  {
    DoutEntering(dc::vkframe, "Window::DrawSample() [" << this << "]");
    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;

    auto swapchain_extent = swapchain().extent();
    main_pass.update_image_views(swapchain(), frame_resources);
#if ENABLE_IMGUI
    imgui_pass.update_image_views(swapchain(), frame_resources);
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

    float scaling_factor = static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height);

    m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });
    auto command_buffer = frame_resources->m_command_buffer;

    Dout(dc::vkframe, "Start recording command buffer.");
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    command_buffer->beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
if (!m_vh_graphics_pipeline)
Dout(dc::warning, "Pipeline not available");
else {
    command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, m_vh_graphics_pipeline);
    command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { *m_descriptor_set.m_handle }, {});
    command_buffer->bindVertexBuffers(0, { m_vertex_buffers[0].m_vh_buffer, m_vertex_buffers[1].m_vh_buffer }, { 0, 0 });
    command_buffer->setViewport(0, { viewport });
    command_buffer->pushConstants(*m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ), &scaling_factor);
    command_buffer->setScissor(0, { scissor });
    command_buffer->draw(6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, m_sample_parameters.ObjectCount, 0, 0);
}
    command_buffer->endRenderPass();
#if ENABLE_IMGUI
    command_buffer->beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
    m_imgui.render_frame(command_buffer, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
    command_buffer->endRenderPass();
#endif
    command_buffer->end();
    Dout(dc::vkframe, "End recording command buffer.");

    submit(command_buffer);

    Dout(dc::vkframe, "Leaving Window::DrawSample.");
  }

  //===========================================================================
  //
  // ImGui
  //
  //===========================================================================

  FrameResourcesCount const& application() const
  {
    return static_cast<FrameResourcesCount const&>(task::SynchronousWindow::application());
  }

  void draw_imgui() override final
  {
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();

    //  bool show_demo_window = true;
    //  ShowDemoWindow(&show_demo_window);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_timer);

    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f));
    ImGui::Begin(reinterpret_cast<char const*>(application().application_name().c_str()), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    static std::string const hardware_name = "Hardware: " + static_cast<std::string>(logical_device()->vh_physical_device().getProperties().deviceName);
    ImGui::Text("%s", hardware_name.c_str());
    ImGui::NewLine();
    ImGui::SliderInt("Scene complexity", &m_sample_parameters.ObjectCount, 10, m_sample_parameters.s_max_object_count);
    ImGui::SliderInt("Frame resources count", &m_sample_parameters.FrameResourcesCount, 1, static_cast<int>(m_frame_resources_list.size()));
    ImGui::SliderInt("Pre-submit CPU work time [ms]", &m_sample_parameters.PreSubmitCpuWorkTime, 0, 20);
    ImGui::SliderInt("Post-submit CPU work time [ms]", &m_sample_parameters.PostSubmitCpuWorkTime, 0, 20);
    ImGui::Text("Frame generation time: %5.2f ms", m_sample_parameters.m_frame_generation_time);
    ImGui::Text("Total frame time: %5.2f ms", m_sample_parameters.m_total_frame_time);

    ImGui::End();
  }

  virtual bool is_slow() const
  {
    return false;
  }
};
