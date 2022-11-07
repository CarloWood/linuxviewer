#pragma once

#include "HeavyRectangle.h"
#include "RandomPositions.h"
#include "PushConstant.h"
#include "SampleParameters.h"
#include "FrameResourcesCount.h"
#include "queues/CopyDataToBuffer.h"
#include "queues/CopyDataToImage.h"
#include "vulkan/SynchronousWindow.h"
#include "vulkan/Pipeline.h"
#include "vulkan/shader_builder/ShaderIndex.h"
#include "vk_utils/ImageData.h"
#include "utils/threading/aithreadid.h"
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
  // Additional image (view) kind.
//  static vulkan::ImageKind const s_vector_image_kind;
//  static vulkan::ImageViewKind const s_vector_image_view_kind;

  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth",    s_depth_image_view_kind};
//  Attachment   position{this, "position", s_vector_image_view_kind};
//  Attachment     normal{this, "normal",   s_vector_image_view_kind};
//  Attachment     albedo{this, "albedo",   s_color_image_view_kind};

  vulkan::shader_builder::ShaderIndex m_shader_vert;
  vulkan::shader_builder::ShaderIndex m_shader_frag;

  // Vertex buffers.
  using vertex_buffers_container_type = std::vector<vulkan::memory::Buffer>;
  using vertex_buffers_type = aithreadsafe::Wrapper<vertex_buffers_container_type, aithreadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;
  mutable vertex_buffers_type m_vertex_buffers; // threadsafe- const.

 private:
  vulkan::shader_resource::Texture m_background_texture{"m_background_texture"};
  vulkan::shader_resource::Texture m_benchmark_texture{"m_benchmark_texture"};
  vulkan::Pipeline m_graphics_pipeline;

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

  vulkan::FrameResourceIndex max_number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{5};
  }

  vulkan::SwapchainIndex max_number_of_swapchain_images() const override
  {
    return vulkan::SwapchainIndex{8};
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    // Background texture.
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / "textures/background.png", 4);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const background_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const background_image_view_kind(background_image_kind, {});

        m_background_texture =
          vulkan::shader_resource::Texture(
              "background",
              m_logical_device,
              texture_data.extent(), background_image_view_kind,
              { .mipmapMode = vk::SamplerMipmapMode::eNearest,
                .anisotropyEnable = VK_FALSE },
              graphics_settings(),
              { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
              COMMA_CWDEBUG_ONLY(debug_name_prefix(".m_background_texture")));

        auto copy_data_to_image = statefultask::create<task::CopyDataToImage>(m_logical_device, texture_data.size(),
            m_background_texture.m_vh_image, texture_data.extent(), vk_defaults::ImageSubresourceRange{},
            vk::ImageLayout::eUndefined, vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
            COMMA_CWDEBUG_ONLY(true));

        copy_data_to_image->set_resource_owner(this);   // Wait for this task to finish before destroying this window, because this window owns the texture (m_background_texture).
        copy_data_to_image->set_data_feeder(std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)));
        copy_data_to_image->run(vulkan::Application::instance().low_priority_queue());
      }
    }

    // Sample texture.
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / "textures/frame_resources.png", 4);
      // Create descriptor resources.
      {
        static vulkan::ImageKind const sample_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

        m_benchmark_texture = vulkan::shader_resource::Texture(
            "benchmark",
            m_logical_device,
            texture_data.extent(), sample_image_view_kind,
            { .mipmapMode = vk::SamplerMipmapMode::eNearest,
              .anisotropyEnable = VK_FALSE },
            graphics_settings(),
            { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_benchmark_texture")));

        auto copy_data_to_image = statefultask::create<task::CopyDataToImage>(m_logical_device, texture_data.size(),
            m_benchmark_texture.m_vh_image, texture_data.extent(), vk_defaults::ImageSubresourceRange{},
            vk::ImageLayout::eUndefined, vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
            COMMA_CWDEBUG_ONLY(true));

        copy_data_to_image->set_resource_owner(this);   // Wait for this task to finish before destroying this window, because this window owns the texture (m_benchmark_texture).
        copy_data_to_image->set_data_feeder(std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)));
        copy_data_to_image->run(vulkan::Application::instance().low_priority_queue());
      }
    }
  }

  static constexpr std::string_view intel_vert_glsl = R"glsl(
out gl_PerVertex
{
  vec4 gl_Position;
};

layout(location = 0) out vec2 v_Texcoord;
layout(location = 1) out float v_Distance;

void main()
{
  v_Texcoord = VertexData::m_texture_coordinates;
  v_Distance = 1.0 - InstanceData::m_position[1].z;     // Darken with distance.

  vec4 position = VertexData::m_position[1];
  // Use PushConstant::pc5
  position.y *= PushConstant::aspect_scale;             // Adjust to screen aspect ration.
  position.xy *= pow(v_Distance, 0.5);                  // Scale with distance.
  gl_Position = position + InstanceData::m_position[1];
}
)glsl";

  static constexpr std::string_view intel_frag_glsl = R"glsl(
layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) in float v_Distance;

layout(location = 0) out vec4 o_Color;

void main()
{
  // Use PushConstant::pc2
  // Use PushConstant::pc4
  vec4 background_image = texture(Texture::background, v_Texcoord);
  vec4 benchmark_image = texture(Texture::benchmark, v_Texcoord);
  o_Color = v_Distance * mix(background_image, benchmark_image, benchmark_image.a);
}
)glsl";

  void register_shader_templates() override
  {
    DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");

    using namespace vulkan::shader_builder;

    std::vector<ShaderInfo> shader_info = {
      { vk::ShaderStageFlagBits::eVertex,   "intel.vert.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "intel.frag.glsl" }
    };
    shader_info[0].load(intel_vert_glsl);
    shader_info[1].load(intel_frag_glsl);
    auto indices = application().register_shaders(std::move(shader_info));
    m_shader_vert = indices[0];
    m_shader_frag = indices[1];
  }

  void add_shader_resources_to(vulkan::pipeline::ShaderInputData& shader_input_data) const
  {
    shader_input_data.add_texture(m_background_texture);
    shader_input_data.add_texture(m_benchmark_texture);
  }

  class FrameResourcesCountPipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
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
    std::vector<vk::PushConstantRange> m_push_constant_ranges;

    // Define pipeline objects.
    HeavyRectangle m_heavy_rectangle;           // A rectangle with many vertices.
    RandomPositions m_random_positions;         // Where to put those rectangles.

   protected:
    using direct_base_type = vulkan::pipeline::Characteristic;

    // The different states of this task.
    enum FrameResourcesCountPipelineCharacteristic_state_type {
      FrameResourcesCountPipelineCharacteristic_initialize = direct_base_type::state_end
    };

    ~FrameResourcesCountPipelineCharacteristic() override
    {
      DoutEntering(dc::vulkan, "FrameResourcesCountPipelineCharacteristic::~FrameResourcesCountPipelineCharacteristic() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = FrameResourcesCountPipelineCharacteristic_initialize + 1;

    FrameResourcesCountPipelineCharacteristic(task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)) { }

   protected:
    char const* state_str_impl(state_type run_state) const override
    {
      switch(run_state)
      {
        AI_CASE_RETURN(FrameResourcesCountPipelineCharacteristic_initialize);
      }
      return direct_base_type::state_str_impl(run_state);
    }

    void initialize_impl() override
    {
      set_state(FrameResourcesCountPipelineCharacteristic_initialize);
    }

    void multiplex_impl(state_type run_state) override
    {
      switch (run_state)
      {
        case FrameResourcesCountPipelineCharacteristic_initialize:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Register the vectors that we will fill.
          m_flat_create_info->add(&m_vertex_input_binding_descriptions);
          m_flat_create_info->add(&m_vertex_input_attribute_descriptions);
          m_flat_create_info->add(&shader_input_data().shader_stage_create_infos());
          m_flat_create_info->add(&m_pipeline_color_blend_attachment_states);
          m_flat_create_info->add(&m_dynamic_states);
          m_flat_create_info->add_descriptor_set_layouts(&shader_input_data().sorted_descriptor_set_layouts());
          m_flat_create_info->add(&m_push_constant_ranges);

          // Define the pipeline.
          shader_input_data().add_vertex_input_binding(m_heavy_rectangle);
          shader_input_data().add_vertex_input_binding(m_random_positions);
          shader_input_data().add_push_constant<PushConstant>();
          window->add_shader_resources_to(shader_input_data());

          {
            using namespace vulkan::shader_builder;

            ShaderIndex shader_vert_index = window->m_shader_vert;
            ShaderIndex shader_frag_index = window->m_shader_frag;

            shader_input_data().preprocess1(m_owning_window->application().get_shader_info(shader_vert_index));
            shader_input_data().preprocess1(m_owning_window->application().get_shader_info(shader_frag_index));

            // Compile the shaders.
            m_flat_create_info->add_set_index_hint_map_callback(
                [=, this](vulkan::descriptor::SetIndexHintMap const& set_index_hint_map)
                {
                  Dout(dc::vulkan, "Calling set_index_hint_callback lambda with " << set_index_hint_map << " [" << this << "]");
                  ShaderCompiler compiler;

                  shader_input_data().build_shader(m_owning_window, shader_vert_index, compiler, set_index_hint_map
                      COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));
                  shader_input_data().build_shader(m_owning_window, shader_frag_index, compiler, set_index_hint_map
                      COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));
                });
          }

          m_vertex_input_binding_descriptions = shader_input_data().vertex_binding_descriptions();
          m_vertex_input_attribute_descriptions = shader_input_data().vertex_input_attribute_descriptions();
          m_push_constant_ranges = shader_input_data().push_constant_ranges();

          m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

          // Generate vertex buffers.
          // FIXME: it seems weird to call this here, because create_vertex_buffers should only be called once
          // while the current function is part of a pipeline factory...
          static std::thread::id s_id;
          ASSERT(aithreadid::is_single_threaded(s_id));     // Fails if more than one thread executes this line.
          window->create_vertex_buffers(this);

          shader_input_data().realize_descriptor_set_layouts(m_owning_window->logical_device());

          set_continue_state(Characteristic_fill);
          run_state = CharacteristicRange_initialized;
          break;
        }
      }
      direct_base_type::multiplex_impl(run_state);
    }

#ifdef CWDEBUG
   public:
    void print_on(std::ostream& os) const override
    {
      os << "{ (FrameResourcesCountPipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    auto pipeline_factory = create_pipeline_factory(m_graphics_pipeline, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory.add_characteristic<FrameResourcesCountPipelineCharacteristic>(this COMMA_CWDEBUG_ONLY(true));
    pipeline_factory.generate(this);
  }

  // This member function only uses m_vertex_buffers, which is aithreadsafe.
  // Therefore, the function as a whole is made threadsafe-const
  void create_vertex_buffers(vulkan::pipeline::CharacteristicRange const* pipeline_owner) /*threadsafe-*/ const
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers(" << pipeline_owner << ") [" << this << "]");

    for (vulkan::shader_builder::VertexShaderInputSetBase* vertex_shader_input_set : pipeline_owner->shader_input_data().vertex_shader_input_sets())
    {
      size_t entry_size = vertex_shader_input_set->chunk_size();
      int count = vertex_shader_input_set->chunk_count();
      size_t buffer_size = count * entry_size;

      vk::Buffer new_buffer;
      {
        vertex_buffers_type::wat vertex_buffers_w(m_vertex_buffers);

        vertex_buffers_w->push_back(vulkan::memory::Buffer{logical_device(), buffer_size,
            { .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
              .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_vertex_buffers[" + std::to_string(vertex_buffers_w->size()) + "]"))});

        new_buffer = vertex_buffers_w->back().m_vh_buffer;
      }

      auto copy_data_to_buffer = statefultask::create<task::CopyDataToBuffer>(logical_device(), buffer_size, new_buffer, 0, vk::AccessFlags(0),
          vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput
          COMMA_CWDEBUG_ONLY(true));

      copy_data_to_buffer->set_resource_owner(this);    // Wait for this task to finish before destroying this window, because this window owns the buffer (m_vertex_buffers.back()).
      copy_data_to_buffer->set_data_feeder(std::make_unique<vulkan::shader_builder::VertexShaderInputSetFeeder>(vertex_shader_input_set, pipeline_owner));
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
    // Limit the frame rate of this window to 1000 frames per second.
    return threadpool::Interval<1, std::chrono::milliseconds>{};
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

  void render_frame() override
  {
    DoutEntering(dc::vkframe, "Window::render_frame() [frame:" << m_frame_count << "; " << this << "; Window]");

    // Skip the first frame.
    if (++m_frame_count == 1)
      return;

    ZoneScopedN("Window::render_frame");

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

      {
        // Perform calculation influencing current frame.
        ZoneScopedNC("PerformHardcoreCalculations-pre", 0x20829C);      // Color: jelly bean (blue/cyan-ish).
        PerformHardcoreCalculations(m_sample_parameters.PreSubmitCpuWorkTime);
      }

      // Draw sample-specific data - includes command buffer submission!!
      draw_frame();

      {
        // Perform calculations influencing rendering of a next frame.
        ZoneScopedNC("PerformHardcoreCalculations-post", 0x209C82);     // Color:: jungle green.
        PerformHardcoreCalculations(m_sample_parameters.PostSubmitCpuWorkTime);
      }

      auto frame_generation_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_generation_begin_time);
      float float_frame_generation_time = static_cast<float>(frame_generation_time.count() * 0.001f);
      m_sample_parameters.m_frame_generation_time = m_sample_parameters.m_frame_generation_time * 0.99f + float_frame_generation_time * 0.01f;
    }

    // Draw GUI and present swapchain image.
    finish_frame();

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    float float_frame_time = static_cast<float>(total_frame_time.count() * 0.001f);
    m_sample_parameters.m_total_frame_time = m_sample_parameters.m_total_frame_time * 0.99f + float_frame_time * 0.01f;

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

    float scaling_factor = static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height);

    wait_command_buffer_completed();
    m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });
    auto command_buffer = frame_resources->m_command_buffer;

    Dout(dc::vkframe, "Start recording command buffer.");
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
#if 0
      CwTracyVkZone(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(),
          tracy::IndexPair(max_number_of_frame_resources(), vulkan::SwapchainIndex{0}, max_number_of_swapchain_images()),
          tracy::IndexPair(m_current_frame.m_resource_index, swapchain_index, max_number_of_swapchain_images()));
#endif
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass1, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass2, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);

      command_buffer->beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
if (!m_graphics_pipeline.handle())
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline.handle()));
      command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline.layout(), 0 /* uint32_t first_set */, m_graphics_pipeline.vhv_descriptor_sets(m_current_frame.m_resource_index), {});
      {
        vertex_buffers_type::rat vertex_buffers_r(m_vertex_buffers);
        vertex_buffers_container_type const& vertex_buffers(*vertex_buffers_r);
        command_buffer->bindVertexBuffers(0 /* uint32_t first_binding */, { vertex_buffers[0].m_vh_buffer, vertex_buffers[1].m_vh_buffer }, { 0, 0 });
      }
      command_buffer->setViewport(0, { viewport });
      //FIXME: this should become something like: update_push_constant(scaling_factor, command_buffer);
      command_buffer->pushConstants(m_graphics_pipeline.layout(), vk::ShaderStageFlagBits::eVertex|vk::ShaderStageFlagBits::eFragment, offsetof(PushConstant, aspect_scale), sizeof(float), &scaling_factor);
      command_buffer->setScissor(0, { scissor });
      command_buffer->draw(6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, m_sample_parameters.ObjectCount, 0, 0);
}
      command_buffer->endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#if ENABLE_IMGUI
    {
#if 0
      CwTracyVkZone(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(),
          tracy::IndexPair(max_number_of_frame_resources(), vulkan::SwapchainIndex{0}, max_number_of_swapchain_images()),
          tracy::IndexPair(m_current_frame.m_resource_index, swapchain_index, max_number_of_swapchain_images()));
#endif
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

  FrameResourcesCount const& application() const
  {
    return static_cast<FrameResourcesCount const&>(task::SynchronousWindow::application());
  }

  void draw_imgui() override final
  {
    ZoneScopedN("Window::draw_imgui");
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();
    int current_SwapchainCount = m_sample_parameters.SwapchainCount;

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
    ImGui::SliderInt("Swapchain image count", &m_sample_parameters.SwapchainCount, 1, max_number_of_swapchain_images().get_value());
    ImGui::SliderInt("Frame resources count", &m_sample_parameters.FrameResourcesCount, 1, max_number_of_frame_resources().get_value());
    ImGui::SliderInt("Pre-submit CPU work time [ms]", &m_sample_parameters.PreSubmitCpuWorkTime, 0, 20);
    ImGui::SliderInt("Post-submit CPU work time [ms]", &m_sample_parameters.PostSubmitCpuWorkTime, 0, 20);
    ImGui::Text("Frame generation time: %5.2f ms", m_sample_parameters.m_frame_generation_time);
    ImGui::Text("Total frame time: %5.2f ms", m_sample_parameters.m_total_frame_time);
    ImGui::End();

    if (current_SwapchainCount != m_sample_parameters.SwapchainCount)
      change_number_of_swapchain_images(m_sample_parameters.SwapchainCount);
  }
};
