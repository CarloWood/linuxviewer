#pragma once

#include "TextureTest.h"
#include "Square.h"
#include "TopBottomPositions.h"
#include "InstanceData.h"
#include "SynchronousWindow.h"
#include "PushConstant.h"
#include "Pipeline.h"
#include "VertexBuffers.h"
#include "PushConstantRange.h"
#include "queues/CopyDataToImage.h"
#include "queues/CopyDataToBuffer.h"
#include "descriptor/SetKeyPreference.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "shader_builder/shader_resource/CombinedImageSampler.h"
#include "pipeline/FactoryCharacteristicId.h"
#include "pipeline/AddVertexShader.h"
#include "pipeline/AddFragmentShader.h"
#include "pipeline/Characteristic.h"
#include "vk_utils/ImageData.h"
#include "statefultask/AITimer.h"

#include "pipeline/AddPushConstant.inl.h"
#include "CommandBuffer.inl.h"

#include <imgui.h>
#include "debug.h"
#include "tracy/CwTracy.h"
#ifdef TRACY_ENABLE
#include "tracy/SourceLocationDataIterator.h"
#endif

#define ENABLE_IMGUI 1
#define SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC 1

class Window : public vulkan::task::SynchronousWindow
{
 private:
  using Directory = vulkan::Directory;

 public:
  using vulkan::task::SynchronousWindow::SynchronousWindow;
  static constexpr condition_type texture_uploaded = free_condition;

 private:
  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth", s_depth_image_view_kind};

  static constexpr int number_of_pipeline_factories = 2;
  static constexpr int number_of_combined_image_samplers = 3;
  static constexpr std::array<char const*, number_of_combined_image_samplers> glsl_id_postfixes{ "top", "bottom0", "bottom1" };
  using combined_image_samplers_t = std::array<vulkan::shader_builder::shader_resource::CombinedImageSampler, 3>;
  combined_image_samplers_t m_combined_image_samplers;
  std::array<vulkan::Texture, number_of_combined_image_samplers> m_textures;

  enum class LocalShaderIndex {
    vertex0,
    frag0,
    frag1
  };
  utils::Array<vulkan::shader_builder::ShaderIndex, 3, LocalShaderIndex> m_shader_indices;

  // Vertex buffers.
  vulkan::VertexBuffers m_vertex_buffers;

  // Vertex buffer generators.
  Square m_square;                              // Vertex buffer.
  TopBottomPositions m_top_bottom_positions;    // Instance buffer.

  // Push constant ranges.
  vulkan::PushConstantRange m_push_constant_range_x_position{
    typeid(PushConstant), offsetof(PushConstant, m_x_position), sizeof(PushConstant::m_x_position)};
  vulkan::PushConstantRange m_push_constant_range_texture_index{
    typeid(PushConstant), offsetof(PushConstant, m_texture_index), sizeof(PushConstant::m_texture_index)};

  std::array<vulkan::Pipeline, number_of_pipeline_factories> m_graphics_pipelines;

  imgui::StatsWindow m_imgui_stats_window;
  int m_frame_count = 0;

 private:
  void create_render_graph() override
  {
    DoutEntering(dc::notice, "Window::create_render_graph() [" << this << "]");

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

  boost::intrusive_ptr<AITimer> m_timer;
  int m_loop_var;

 public:
  ~Window() { if (m_timer) m_timer->abort(); }

 private:
  void create_textures() override
  {
    DoutEntering(dc::notice, "Window::create_textures() [" << this << "]");

    std::array<char const*, number_of_combined_image_samplers> textures_names{
      "textures/digit13.png",
      "textures/digitg22.png",
      "textures/digit01.png"
#if 0
      "textures/cat-tail-nature-grass-summer-whiskers-826101-wallhere.com.jpg",
      "textures/nature-grass-sky-insect-green-Izmir-839795-wallhere.com.jpg",
      "textures/tileable10b.png",
      "textures/Tileable5.png"
#endif
    };

    std::string const name_prefix("m_textures[");

    for (int t = 0; t < number_of_combined_image_samplers; ++t)
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / textures_names[t], 4);

      m_textures[t] = vulkan::Texture(m_logical_device,
          texture_data.extent(),
          { .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .anisotropyEnable = VK_FALSE },
          graphics_settings(),
          { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
          COMMA_CWDEBUG_ONLY(debug_name_prefix(name_prefix + glsl_id_postfixes[t] + ']')));

      m_textures[t].upload(texture_data.extent(), this,
          std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)), this, texture_uploaded);
    }

    m_timer = statefultask::create<AITimer>(CWDEBUG_ONLY(true));
    m_loop_var = 0;
    m_timer->set_interval(threadpool::Interval<200, std::chrono::milliseconds>());
    m_timer->run([this](bool success){
      if (!success)
        return;
      for (;;)
      {
        int pipeline_factory = m_loop_var / number_of_combined_image_samplers;
        int cis = m_loop_var % number_of_combined_image_samplers;
        if (cis > 0 && cis != pipeline_factory + 1)
        {
          if (++m_loop_var == number_of_pipeline_factories * number_of_combined_image_samplers)
            break;
          continue;
        }
        int ti = (cis + 1) % number_of_combined_image_samplers;
        // Change (update) image sampler descriptor 'cis', used by pipeline factory 'pipeline_factory', with the (new) texture 'ti'.
#if SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
        int const characteristic = 2; // FragmentPipelineCharacteristicRange.
#else
        int const characteristic = 1; // VertexPipelineCharacteristicRange which now also do the fragment shader.
#endif
        Dout(dc::notice, "Calling update_image_sampler_array with cis = " << cis << "; ti = " << ti << "; characteristic = " << characteristic);
        m_combined_image_samplers[cis].update_image_sampler_array(
            &m_textures[ti], m_pipeline_factory_characteristic_range_ids[pipeline_factory * number_of_characteristics + characteristic], 1);
        break;
      }
      if (++m_loop_var < number_of_pipeline_factories * number_of_combined_image_samplers)
        m_timer->run();
    });
  }

  void create_vertex_buffers() override
  {
    DoutEntering(dc::notice, "Window::create_vertex_buffers() [" << this << "]");

    m_vertex_buffers.create_vertex_buffer(this, m_square);
    m_vertex_buffers.create_vertex_buffer(this, m_top_bottom_positions);
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

  static constexpr std::string_view squares_frag0_glsl = R"glsl(
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) flat in int instance_index;
layout(location = 0) out vec4 outColor;

void main()
{
  if (instance_index == 0)
    outColor = texture(CombinedImageSampler::top[PushConstant::m_texture_index], v_Texcoord);
  else
    outColor = texture(CombinedImageSampler::bottom0[PushConstant::m_texture_index], v_Texcoord);
}
)glsl";

  static constexpr std::string_view squares_frag1_glsl = R"glsl(
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) flat in int instance_index;
layout(location = 0) out vec4 outColor;

void main()
{
  if (instance_index == 0)
    outColor = texture(CombinedImageSampler::top[PushConstant::m_texture_index], v_Texcoord);
  else
    outColor = texture(CombinedImageSampler::bottom1[PushConstant::m_texture_index], v_Texcoord);
}
)glsl";

  void register_shader_templates() override
  {
    DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");

    using namespace vulkan::shader_builder;

    // Create a ShaderInfo instance for each shader, initializing it with the stage that the shader will be used in and the template code that it exists of.
    std::vector<ShaderInfo> shader_info = {
      { vk::ShaderStageFlagBits::eVertex,   "squares.vert.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "squares.frag0.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "squares.frag1.glsl" }
    };
    shader_info[0].load(squares_vert_glsl);
    shader_info[1].load(squares_frag0_glsl);
    shader_info[2].load(squares_frag1_glsl);

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

  class BasePipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
    std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };
    int m_pipeline_factory;

   protected:
    ~BasePipelineCharacteristic() override
    {
      DoutEntering(dc::notice, "BasePipelineCharacteristic::~BasePipelineCharacteristic() [" << this << "]");
    }

   public:
    BasePipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window, int pipeline_factory COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)), m_pipeline_factory(pipeline_factory)
    {
      DoutEntering(dc::notice, "BasePipelineCharacteristic::BasePipelineCharacteristic(" << owning_window << ", " << pipeline_factory << ") [" << this << "]");
    }

   protected:
    void initialize() final
    {
      DoutEntering(dc::notice, "BasePipelineCharacteristic::initialize() [" << this << "]");
      Window const* window = static_cast<Window const*>(m_owning_window);

      // Register the vectors that we will fill.
      add_to_flat_create_info(m_pipeline_color_blend_attachment_states);
      add_to_flat_create_info(m_dynamic_states);

      // Add default color blend.
      m_pipeline_color_blend_attachment_states.push_back(vk_defaults::PipelineColorBlendAttachmentState{});
      // Add default topology.
      m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (BasePipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  class VertexPipelineCharacteristicRange : public vulkan::task::CharacteristicRange,
      public vulkan::pipeline::AddVertexShader,
#if !SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
      public vulkan::pipeline::AddFragmentShader,
#endif
      public vulkan::pipeline::AddPushConstant
  {
   private:
    int m_pipeline_factory;

   protected:
    using direct_base_type = vulkan::task::CharacteristicRange;

    // The different states of this task.
    enum VertexPipelineCharacteristicRange_state_type {
      VertexPipelineCharacteristicRange_initialize = direct_base_type::state_end,
      VertexPipelineCharacteristicRange_fill
    };

    ~VertexPipelineCharacteristicRange() override
    {
      DoutEntering(dc::notice, "VertexPipelineCharacteristicRange::~VertexPipelineCharacteristicRange() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = VertexPipelineCharacteristicRange_fill + 1;

    VertexPipelineCharacteristicRange(vulkan::task::SynchronousWindow const* owning_window, int pipeline_factory COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::task::CharacteristicRange(owning_window, 0, 2 COMMA_CWDEBUG_ONLY(debug)), m_pipeline_factory(pipeline_factory)
    {
      DoutEntering(dc::notice, "VertexPipelineCharacteristicRange::VertexPipelineCharacteristicRange(" << owning_window << ", " << pipeline_factory << ") [" << this << "]");
    }

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
        AI_CASE_RETURN(VertexPipelineCharacteristicRange_initialize);
        AI_CASE_RETURN(VertexPipelineCharacteristicRange_fill);
      }
      return direct_base_type::state_str_impl(run_state);
    }

    void initialize_impl() override
    {
      set_state(VertexPipelineCharacteristicRange_initialize);
    }

    void multiplex_impl(state_type run_state) override
    {
      switch (run_state)
      {
        case VertexPipelineCharacteristicRange_initialize:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Define the pipeline.
          add_push_constant<PushConstant>(window->m_push_constant_range_x_position);
          add_vertex_input_bindings(window->m_vertex_buffers);        // Filled in create_vertex_buffers
#if !SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
          add_combined_image_sampler(window->m_combined_image_samplers[0]);
          add_combined_image_sampler(window->m_combined_image_samplers[m_pipeline_factory + 1]);
#endif

          set_fill_state(VertexPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_initialized;
          break;
        }
        case VertexPipelineCharacteristicRange_fill:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);
          Dout(dc::notice, "fill_index = " << fill_index());

          if (fill_index() == 0)
          {
            using namespace vulkan::shader_builder;
            ShaderIndex vertex_shader_index = window->m_shader_indices[LocalShaderIndex::vertex0];
            add_shader(vertex_shader_index);
#if !SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
            ShaderIndex fragment_shader_index = window->m_shader_indices[m_pipeline_factory == 0 ? LocalShaderIndex::frag0 : LocalShaderIndex::frag1];
            add_shader(fragment_shader_index);
#endif
          }
          // Preprocess and build the shaders that were just passed to add_shader()
          // and then generate the next pipeline from the current state.
          run_state = CharacteristicRange_filled;
          break;
        }
      }
      direct_base_type::multiplex_impl(run_state);
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (VertexPipelineCharacteristicRange*)" << this << " }";
    }
#endif
  };

#if SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
  class FragmentPipelineCharacteristicRange : public vulkan::task::CharacteristicRange,
      public vulkan::pipeline::AddFragmentShader,
      public vulkan::pipeline::AddPushConstant
  {
   private:
    int m_pipeline_factory;

   protected:
    using direct_base_type = vulkan::task::CharacteristicRange;

    // The different states of this task.
    enum FragmentPipelineCharacteristicRange_state_type {
      FragmentPipelineCharacteristicRange_initialize = direct_base_type::state_end,
      FragmentPipelineCharacteristicRange_fill
    };

    ~FragmentPipelineCharacteristicRange() override
    {
      DoutEntering(dc::notice, "FragmentPipelineCharacteristicRange::~FragmentPipelineCharacteristicRange() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = FragmentPipelineCharacteristicRange_fill + 1;

    FragmentPipelineCharacteristicRange(vulkan::task::SynchronousWindow const* owning_window, int pipeline_factory COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::task::CharacteristicRange(owning_window, 0, 2 COMMA_CWDEBUG_ONLY(debug)), m_pipeline_factory(pipeline_factory)
    {
      DoutEntering(dc::notice, "FragmentPipelineCharacteristicRange::FragmentPipelineCharacteristicRange(" << owning_window << ", " << pipeline_factory << ") [" << this << "]");
    }

   protected:
    char const* state_str_impl(state_type run_state) const override
    {
      switch(run_state)
      {
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_initialize);
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_fill);
      }
      return direct_base_type::state_str_impl(run_state);
    }

    void initialize_impl() override
    {
      set_state(FragmentPipelineCharacteristicRange_initialize);
    }

    void multiplex_impl(state_type run_state) override
    {
      switch (run_state)
      {
        case FragmentPipelineCharacteristicRange_initialize:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Define the pipeline.
          add_push_constant<PushConstant>(window->m_push_constant_range_texture_index);
#if 1
          // Below we use 1 + m_pipeline_factory as index into the array of combined image samplers.
          ASSERT(number_of_combined_image_samplers >= number_of_pipeline_factories);
#if 1
          std::vector<vulkan::descriptor::SetKeyPreference> key_preference;
          for (int t = 0; t < number_of_combined_image_samplers; ++t)
            key_preference.emplace_back(window->m_combined_image_samplers[t].descriptor_task()->descriptor_set_key(), 0.1);
#endif

          int constexpr number_of_combined_image_samplers_per_pipeline = 2;
          std::array<int, number_of_combined_image_samplers_per_pipeline> combined_image_sampler_indexes = {
            0,
            1 + m_pipeline_factory
          };
          for (int i = 0; i < number_of_combined_image_samplers_per_pipeline; ++i)
          {
            add_combined_image_sampler(
                window->m_combined_image_samplers[combined_image_sampler_indexes[i]]
#if 1
                , {}
                , { key_preference[combined_image_sampler_indexes[1 - i]] }
#endif
                );
          }
#else
          add_combined_image_sampler(window->m_combined_image_samplers[0]);
          add_combined_image_sampler(window->m_combined_image_samplers[m_pipeline_factory + 1]);
#endif

          set_fill_state(FragmentPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_initialized;
          break;
        }
        case FragmentPipelineCharacteristicRange_fill:
        {
          using namespace vulkan::shader_builder;
          Dout(dc::notice, "fill_index = " << fill_index());
          Window const* window = static_cast<Window const*>(m_owning_window);
          ShaderIndex fragment_shader_index = window->m_shader_indices[m_pipeline_factory == 0 ? LocalShaderIndex::frag0 : LocalShaderIndex::frag1];
          if (fill_index() == 0)
            add_shader(fragment_shader_index);
          // Preprocess and build the shaders that were just passed to add_shader()
          // and then generate the next pipeline from the current state.
          run_state = CharacteristicRange_filled;
          break;
        }
      }
      direct_base_type::multiplex_impl(run_state);
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (FragmentPipelineCharacteristicRange*)" << this << " }";
    }
#endif
  };

  static constexpr int number_of_characteristics = 3;
#else
  static constexpr int number_of_characteristics = 2;
#endif

  std::array<vulkan::pipeline::FactoryCharacteristicId, number_of_pipeline_factories * number_of_characteristics> m_pipeline_factory_characteristic_range_ids;
  std::array<vulkan::pipeline::FactoryHandle, number_of_pipeline_factories> m_pipeline_factory;

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::notice, "Window::create_graphics_pipelines() [" << this << "]");
    auto const unbounded_array = vulkan::shader_builder::shader_resource::CombinedImageSampler::unbounded_array;

    for (int t = 0; t < number_of_combined_image_samplers; ++t)
    {
      // This is what the descriptor is recognized by in the shader code.
      m_combined_image_samplers[t].set_glsl_id_postfix(glsl_id_postfixes[t]);
      // This is needed if you want to change the texture again after using it.
      m_combined_image_samplers[t].set_bindings_flags(vk::DescriptorBindingFlagBits::eUpdateAfterBind);
      // Turn this descriptor into an array with size 2, using [] for its declaration in the shader.
      m_combined_image_samplers[t].set_array_size(2, unbounded_array);
    }

    for (int pipeline_factory = 0; pipeline_factory < number_of_pipeline_factories; ++pipeline_factory)
    {
      m_pipeline_factory[pipeline_factory] =
        create_pipeline_factory(m_graphics_pipelines[pipeline_factory], main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
      // We have number_of_characteristics factory/range pair per pipeline factory here.
      for (int c = 0; c < number_of_characteristics; ++c)
      {
        vulkan::pipeline::FactoryCharacteristicId factory_characteristic_id;
        switch (c)
        {
          case 0:
            factory_characteristic_id =
              m_pipeline_factory[pipeline_factory].add_characteristic<BasePipelineCharacteristic>(this, pipeline_factory COMMA_CWDEBUG_ONLY(true));
            break;
          case 1:
            factory_characteristic_id =
              m_pipeline_factory[pipeline_factory].add_characteristic<VertexPipelineCharacteristicRange>(this, pipeline_factory COMMA_CWDEBUG_ONLY(true));
            break;
#if SEPARATE_FRAGMENT_SHADER_CHARACTERISTIC
          case 2:
            factory_characteristic_id =
              m_pipeline_factory[pipeline_factory].add_characteristic<FragmentPipelineCharacteristicRange>(this, pipeline_factory COMMA_CWDEBUG_ONLY(true));
            break;
#endif
        }
        m_pipeline_factory_characteristic_range_ids[pipeline_factory * number_of_characteristics + c] = factory_characteristic_id;
      }

      m_pipeline_factory[pipeline_factory].generate(this);
    }
  }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    DoutEntering(dc::notice, "Window::get_frame_rate_interval() [" << this << "]");
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

    vulkan::handle::CommandBuffer command_buffer = frame_resources->m_command_buffer;
    Dout(dc::vkframe, "Start recording command buffer.");
    command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass1, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass2, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);

      command_buffer.beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
bool have_all_pipelines = true;
for (int pl = 0; pl < number_of_pipeline_factories; ++pl)
  if (!m_graphics_pipelines[pl].handle())
  {
    have_all_pipelines = false;
    break;
  }
if (!have_all_pipelines)
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer.setViewport(0, { viewport });
      command_buffer.setScissor(0, { scissor });
      command_buffer.bindVertexBuffers(m_vertex_buffers);

      glsl::Int texture_index = 1;
      for (int pl = 0; pl < number_of_pipeline_factories; ++pl)
      {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipelines[pl].handle()));
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipelines[pl].layout(), 0 /* uint32_t first_set */,
            m_graphics_pipelines[pl].vhv_descriptor_sets(m_current_frame.m_resource_index), {});

        glsl::Float x_position = pl - 0.5f;
        command_buffer.pushConstants(m_graphics_pipelines[pl].layout(), m_push_constant_range_x_position, x_position);
        command_buffer.pushConstants(m_graphics_pipelines[pl].layout(), m_push_constant_range_texture_index, texture_index);
        command_buffer.draw(6 * square_steps * square_steps, 2, 0, 0);
      }

}
      command_buffer.endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#if ENABLE_IMGUI
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass1, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass2, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);
      command_buffer.beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
      m_imgui.render_frame(command_buffer, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
      command_buffer.endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#endif
    command_buffer.end();
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
    return static_cast<TextureTest const&>(vulkan::task::SynchronousWindow::application());
  }

  void draw_imgui() override final
  {
    ZoneScopedN("Window::draw_imgui");
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_imgui_timer);
  }
};
