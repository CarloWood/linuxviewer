#pragma once

#include "TextureTest.h"
#include "Square.h"
#include "TopBottomPositions.h"
#include "InstanceData.h"
#include "SynchronousWindow.h"
#include "PushConstant.h"
#include "queues/CopyDataToImage.h"
#include "queues/CopyDataToBuffer.h"
#include "Pipeline.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "descriptor/SetKeyPreference.h"
#include "vk_utils/ImageData.h"
#include <imgui.h>
#include "debug.h"
#include "tracy/CwTracy.h"
#ifdef TRACY_ENABLE
#include "tracy/SourceLocationDataIterator.h"
#endif

#define ENABLE_IMGUI 0

class Window : public task::SynchronousWindow
{
 private:
  using Directory = vulkan::Directory;

 public:
  using task::SynchronousWindow::SynchronousWindow;
  static constexpr condition_type texture_uploaded = free_condition;

 private:
  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth", s_depth_image_view_kind};

  static constexpr int number_of_textures = 2;
  std::array<vulkan::shader_resource::Texture, number_of_textures> m_textures = {
    "m_texture_top", "m_texture_bottom"
  };

  enum class LocalShaderIndex {
    vertex0,
    frag0
  };
  utils::Array<vulkan::shader_builder::ShaderIndex, 2, LocalShaderIndex> m_shader_indices;

  // Vertex buffers.
  using vertex_buffers_container_type = std::vector<vulkan::memory::Buffer>;
  using vertex_buffers_type = aithreadsafe::Wrapper<vertex_buffers_container_type, aithreadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;
  mutable vertex_buffers_type m_vertex_buffers; // threadsafe- const.

  static constexpr int number_of_pipelines = 2;
  std::array<vulkan::Pipeline, number_of_pipelines> m_graphics_pipelines;

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

  vulkan::FrameResourceIndex max_number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{2};
  }

  vulkan::SwapchainIndex max_number_of_swapchain_images() const override
  {
    return vulkan::SwapchainIndex{4};
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    std::array<char const*, number_of_textures> textures_names{
      "textures/cat-tail-nature-grass-summer-whiskers-826101-wallhere.com.jpg",
      "textures/nature-grass-sky-insect-green-Izmir-839795-wallhere.com.jpg" //,
#if 0
      "textures/tileable10b.png",
      "textures/Tileable5.png"
#endif
    };

    std::array<char const*, number_of_textures> glsl_id_postfixes{
      "top", "bottom"
    };

    std::string const name_prefix("m_textures[");

    for (int t = 0; t < number_of_textures; ++t)
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / textures_names[t], 4);

      // Create descriptor resources.
      static vulkan::ImageKind const sample_image_kind({
        .format = vk::Format::eR8G8B8A8Unorm,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
      });

      static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

      m_textures[t] = vulkan::shader_resource::Texture(glsl_id_postfixes[t], m_logical_device,
          texture_data.extent(), sample_image_view_kind,
          { .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .anisotropyEnable = VK_FALSE },
          graphics_settings(),
          { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
          COMMA_CWDEBUG_ONLY(debug_name_prefix(name_prefix + glsl_id_postfixes[t] + ']')));

      m_textures[t].upload(texture_data.extent(), sample_image_view_kind, this,
          std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)), this, texture_uploaded);
    }
  }

 private:
  static constexpr std::string_view squares_vert_glsl = R"glsl(
out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out vec2 v_Texcoord;
layout(location = 1) out int instance_index;

void main()
{
  instance_index = gl_InstanceIndex;
  v_Texcoord = VertexData::m_texture_coordinates;
  vec4 position = VertexData::m_position;
  vec4 instance_position = InstanceData::m_position;
  position += instance_position;
  position.x += PushConstant::m_x_position;
  gl_Position = position;
}
)glsl";

  static constexpr std::string_view squares_frag_glsl = R"glsl(
layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) flat in int instance_index;
layout(location = 0) out vec4 outColor;

void main()
{
  int foo = PushConstant::m_texture_index;
  if (instance_index == 0)
    outColor = texture(Texture::top, v_Texcoord);
  else
    outColor = texture(Texture::bottom, v_Texcoord);
}
)glsl";

  void register_shader_templates() override
  {
    DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");

    using namespace vulkan::shader_builder;

    // Create a ShaderInfo instance for each shader, initializing it with the stage that the shader will be used in and the template code that it exists of.
    std::vector<ShaderInfo> shader_info = {
      { vk::ShaderStageFlagBits::eVertex,   "squares.vert.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "squares.frag.glsl" },
    };
    shader_info[0].load(squares_vert_glsl);
    shader_info[1].load(squares_frag_glsl);

    // Inform the application about the shaders that we use.
    // This will call hash() on each ShaderInfo object (which may only called once), and then store it only when that hash doesn't exist yet.
    // Since the hash is calculated over the template code (excluding generated declaration), the declarations are fixed for given template
    // code. The declaration determine the mapping between descriptor set, and binding number, and shader variable.
    auto indices = application().register_shaders(std::move(shader_info));

    // Copy the returned "shader indices" into our local array.
    ASSERT(indices.size() == m_shader_indices.size());
    for (int i = 0; i < indices.size(); ++i)
      m_shader_indices[static_cast<LocalShaderIndex>(i)] = indices[i];
  }

  void add_shader_resources_to(vulkan::pipeline::ShaderInputData& shader_input_data, int pipeline) const
  {
    // Define the pipeline.
    shader_input_data.add_texture(m_textures[0]);
    shader_input_data.add_texture(m_textures[1]);
  }

  class TextureTestPipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
    std::vector<vk::VertexInputBindingDescription> m_vertex_input_binding_descriptions;
    std::vector<vk::VertexInputAttributeDescription> m_vertex_input_attribute_descriptions;
    std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };
    std::vector<vk::PushConstantRange> m_push_constant_ranges;
    int m_pipeline;

    // Define pipeline objects.
    Square m_square;
    TopBottomPositions m_top_bottom_positions;

   protected:
    using direct_base_type = vulkan::pipeline::Characteristic;

    // The different states of this task.
    enum TextureTestPipelineCharacteristic_state_type {
      TextureTestPipelineCharacteristic_initialize = direct_base_type::state_end,
      TextureTestPipelineCharacteristic_compile
    };

    ~TextureTestPipelineCharacteristic() override
    {
      DoutEntering(dc::vulkan, "TextureTestPipelineCharacteristic::~TextureTestPipelineCharacteristic() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = TextureTestPipelineCharacteristic_compile + 1;

    TextureTestPipelineCharacteristic(task::SynchronousWindow const* owning_window, int pipeline COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)), m_pipeline(pipeline) { }

   protected:
    char const* condition_str_impl(condition_type condition) const override
    {
      switch (condition)
      {
      }
      return direct_base_type::condition_str_impl(condition);
    }

    char const* state_str_impl(state_type run_state) const override
    {
      switch(run_state)
      {
        AI_CASE_RETURN(TextureTestPipelineCharacteristic_initialize);
        AI_CASE_RETURN(TextureTestPipelineCharacteristic_compile);
      }
      return direct_base_type::state_str_impl(run_state);
    }

    void initialize_impl() override
    {
      set_state(TextureTestPipelineCharacteristic_initialize);
    }

    void multiplex_impl(state_type run_state) override
    {
      switch (run_state)
      {
        case TextureTestPipelineCharacteristic_initialize:
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
          shader_input_data().add_vertex_input_binding(m_square);
          shader_input_data().add_vertex_input_binding(m_top_bottom_positions);
          shader_input_data().add_push_constant<PushConstant>();
          window->add_shader_resources_to(shader_input_data(), m_pipeline);

          // Add default color blend.
          m_pipeline_color_blend_attachment_states.push_back(vk_defaults::PipelineColorBlendAttachmentState{});

          // Compile the shaders.
          {
            using namespace vulkan::shader_builder;

            ShaderIndex shader_vert_index = window->m_shader_indices[LocalShaderIndex::vertex0];
            ShaderIndex shader_frag_index = window->m_shader_indices[LocalShaderIndex::frag0];

            // These two calls fill ShaderInputData::m_sorted_descriptor_set_layouts with arbitrary binding numbers (in the order that they are found in the shader template code).
            shader_input_data().preprocess1(m_owning_window->application().get_shader_info(shader_vert_index));
            shader_input_data().preprocess1(m_owning_window->application().get_shader_info(shader_frag_index));
          }

          m_vertex_input_binding_descriptions = shader_input_data().vertex_binding_descriptions();
          m_vertex_input_attribute_descriptions = shader_input_data().vertex_input_attribute_descriptions();
          m_push_constant_ranges = shader_input_data().push_constant_ranges();

          m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

          // Generate vertex buffers.
          // FIXME: it seems weird to call this here, because create_vertex_buffers should only be called once
          // while the current function is part of a pipeline factory...
          static std::atomic_int done = false;
          if (done.fetch_or(true) == false)
            window->create_vertex_buffers(this);

          // Realize the descriptor set layouts: if a layout already exists then use the existing
          // handle and update the binding values used in ShaderInputData::m_sorted_descriptor_set_layouts.
          // Otherwise, if it does not already exist, create a new descriptor set layout using the
          // provided binding values as-is.
          shader_input_data().realize_descriptor_set_layouts(m_owning_window->logical_device());

          set_continue_state(TextureTestPipelineCharacteristic_compile);
          run_state = Characteristic_initialized;
          break;
        }
        case TextureTestPipelineCharacteristic_compile:
        {
          using namespace vulkan::shader_builder;
          Window const* window = static_cast<Window const*>(m_owning_window);

          ShaderIndex shader_vert_index = window->m_shader_indices[LocalShaderIndex::vertex0];
          ShaderIndex shader_frag_index = window->m_shader_indices[LocalShaderIndex::frag0];

          // Compile the shaders.
          ShaderCompiler compiler;
          shader_input_data().build_shader(m_owning_window, shader_vert_index, compiler, m_set_index_hint_map
              COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));
          shader_input_data().build_shader(m_owning_window, shader_frag_index, compiler, m_set_index_hint_map
              COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));

          run_state = Characteristic_compiled;
          break;
        }
      }
      direct_base_type::multiplex_impl(run_state);
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (TextureTestPipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  std::array<vulkan::pipeline::FactoryHandle, number_of_pipelines> m_pipeline_factory;

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    for (int pipeline = 0; pipeline < number_of_pipelines; ++pipeline)
    {
      m_pipeline_factory[pipeline] = create_pipeline_factory(m_graphics_pipelines[pipeline], main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
      m_pipeline_factory[pipeline].add_characteristic<TextureTestPipelineCharacteristic>(this, pipeline COMMA_CWDEBUG_ONLY(true));
      m_pipeline_factory[pipeline].generate(this);
    }
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
    // Limit the frame rate of this window to 100 frames per second.
    return threadpool::Interval<10, std::chrono::milliseconds>{};
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
#if 0
    if (m_frame_count >= 30)
    {
      if (m_frame_count == 30)
        close();
      return;
    }
#endif

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
if (!m_graphics_pipelines[0].handle() || !m_graphics_pipelines[1].handle())
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer->setViewport(0, { viewport });
      command_buffer->setScissor(0, { scissor });
      {
        vertex_buffers_type::rat vertex_buffers_r(m_vertex_buffers);
        vertex_buffers_container_type const& vertex_buffers(*vertex_buffers_r);
        command_buffer->bindVertexBuffers(0 /* uint32_t first_binding */, { vertex_buffers[0].m_vh_buffer, vertex_buffers[1].m_vh_buffer }, { 0, 0 });
      }

      for (int pl = 0; pl < number_of_pipelines; ++pl)
      {
        command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipelines[pl].handle()));
        command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipelines[pl].layout(), 0 /* uint32_t first_set */,
            m_graphics_pipelines[pl].vhv_descriptor_sets(m_current_frame.m_resource_index), {});

        PushConstant pc = {
          pl - 0.5f,
          1
        };
        command_buffer->pushConstants(m_graphics_pipelines[pl].layout(), vk::ShaderStageFlagBits::eVertex, offsetof(PushConstant, m_x_position), sizeof(PushConstant::m_x_position), &pc.m_x_position);
        command_buffer->pushConstants(m_graphics_pipelines[pl].layout(), vk::ShaderStageFlagBits::eFragment, offsetof(PushConstant, m_texture_index), sizeof(PushConstant::m_texture_index), &pc.m_texture_index);
        command_buffer->draw(6 * square_steps * square_steps, 2, 0, 0);
      }

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

  TextureTest const& application() const
  {
    return static_cast<TextureTest const&>(task::SynchronousWindow::application());
  }

  void draw_imgui() override final
  {
    ZoneScopedN("Window::draw_imgui");
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_timer);
  }
};
