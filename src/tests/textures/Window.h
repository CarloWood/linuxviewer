#pragma once

#include "TextureTest.h"
#include "Square.h"
#include "TopBottomPositions.h"
#include "InstanceData.h"
#include "SynchronousWindow.h"
#include "PushConstant.h"
#include "Pipeline.h"
#include "queues/CopyDataToImage.h"
#include "queues/CopyDataToBuffer.h"
#include "descriptor/CombinedImageSamplerUpdater.h"
#include "descriptor/SetKeyPreference.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "shader_builder/shader_resource/CombinedImageSampler.h"
#include "pipeline/FactoryCharacteristicId.h"
#include "pipeline/AddVertexShader.h"
#include "pipeline/AddFragmentShader.h"
#include "vk_utils/ImageData.h"
#include "statefultask/AITimer.h"

#include "pipeline/AddPushConstant.inl.h"

#include <imgui.h>
#include "debug.h"
#include "tracy/CwTracy.h"
#ifdef TRACY_ENABLE
#include "tracy/SourceLocationDataIterator.h"
#endif

#define ENABLE_IMGUI 0
#define SEPARATE_FRAGMENT_SHADER 0

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

  static constexpr int number_of_combined_image_samplers = 1;
  static constexpr std::array<char const*, number_of_combined_image_samplers> glsl_id_postfixes{ "top", /*"bottom0", "bottom1"*/ };
  using combined_image_samplers_t = std::array<vulkan::shader_builder::shader_resource::CombinedImageSampler, number_of_combined_image_samplers>;
  combined_image_samplers_t m_combined_image_samplers;
  std::array<vulkan::Texture, number_of_combined_image_samplers> m_textures;

  enum class LocalShaderIndex {
    vertex0,
    frag0,
    frag1
  };
  utils::Array<vulkan::shader_builder::ShaderIndex, 3, LocalShaderIndex> m_shader_indices;

  // Vertex buffers.
  using vertex_buffers_container_type = std::vector<vulkan::memory::Buffer>;
  using vertex_buffers_type = aithreadsafe::Wrapper<vertex_buffers_container_type, aithreadsafe::policy::ReadWrite<AIReadWriteSpinLock>>;
  mutable vertex_buffers_type m_vertex_buffers; // threadsafe- const.

  static constexpr int number_of_pipelines = 1;
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

  boost::intrusive_ptr<AITimer> m_timer;
  int m_loop_var;
 public:
  ~Window() { if (m_timer) m_timer->abort(); }
 private:
  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    std::array<char const*, number_of_combined_image_samplers> textures_names{
      "textures/digit13.png" //,
#if 0
      "textures/digitg22.png",
      "textures/digit01.png"
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
      for (;;)
      {
        int pipeline = m_loop_var / number_of_combined_image_samplers;
        int cis = m_loop_var % number_of_combined_image_samplers;
        if (cis > 0 && cis != pipeline + 1)
        {
          if (++m_loop_var == number_of_pipelines * number_of_combined_image_samplers)
            break;
          continue;
        }
        int ti = (cis + 1) % number_of_combined_image_samplers;
        // Change (update) image sampler descriptor 'cis', used by pipeline 'pipeline', with the (new) texture 'ti'.
#if SEPARATE_FRAGMENT_SHADER
        int const characteristic = 2; // FragmentPipelineCharacteristicRange.
#else
        int const characteristic = 1; // VertexPipelineCharacteristicRange which now also do the fragment shader.
#endif
        m_combined_image_samplers[cis].update_image_sampler_array(&m_textures[ti], m_pipeline_factory_characteristic_range_ids[pipeline * number_of_characteristics + characteristic], 1);
        break;
      }
      if (++m_loop_var < number_of_pipelines * number_of_combined_image_samplers)
        m_timer->run();
    });
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
//  if (instance_index == 0)
    outColor = texture(CombinedImageSampler::top[PushConstant::m_texture_index], v_Texcoord);
//  else
//    outColor = texture(CombinedImageSampler::bottom0[PushConstant::m_texture_index], v_Texcoord);
}
)glsl";

  static constexpr std::string_view squares_frag1_glsl = R"glsl(
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 v_Texcoord;
layout(location = 1) flat in int instance_index;
layout(location = 0) out vec4 outColor;

void main()
{
//  if (instance_index == 0)
    outColor = texture(CombinedImageSampler::top[PushConstant::m_texture_index], v_Texcoord);
//  else
//    outColor = texture(CombinedImageSampler::bottom1[PushConstant::m_texture_index], v_Texcoord);
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

  // Accessor.
  combined_image_samplers_t const& combined_image_samplers() const { return m_combined_image_samplers; }

  class BasePipelineCharacteristic : public vulkan::pipeline::Characteristic
  {
   private:
    std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };
    int m_pipeline;

   protected:
    using direct_base_type = vulkan::pipeline::Characteristic;

    // The different states of this task.
    enum BasePipelineCharacteristic_state_type {
      BasePipelineCharacteristic_initialize = direct_base_type::state_end,
      BasePipelineCharacteristic_done
    };

    ~BasePipelineCharacteristic() override
    {
      DoutEntering(dc::vulkan, "BasePipelineCharacteristic::~BasePipelineCharacteristic() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = BasePipelineCharacteristic_done + 1;

    BasePipelineCharacteristic(task::SynchronousWindow const* owning_window, int pipeline COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)), m_pipeline(pipeline) { }

   protected:
    char const* state_str_impl(state_type run_state) const override
    {
      switch(run_state)
      {
        AI_CASE_RETURN(BasePipelineCharacteristic_initialize);
        AI_CASE_RETURN(BasePipelineCharacteristic_done);
      }
      return direct_base_type::state_str_impl(run_state);
    }

    void multiplex_impl(state_type run_state) override
    {
      switch (run_state)
      {
        case BasePipelineCharacteristic_initialize:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Register the vectors that we will fill.
          m_flat_create_info->add(&m_pipeline_color_blend_attachment_states);
          m_flat_create_info->add(&m_dynamic_states);

          // Add default color blend.
          m_pipeline_color_blend_attachment_states.push_back(vk_defaults::PipelineColorBlendAttachmentState{});
          // Add default topology.
          m_flat_create_info->m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

          set_continue_state(BasePipelineCharacteristic_done);
          run_state = Characteristic_initialized;
          break;
        }
        case BasePipelineCharacteristic_done:
          finish();
          break;
      }
      direct_base_type::multiplex_impl(run_state);
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (BasePipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  class VertexPipelineCharacteristicRange : public vulkan::pipeline::CharacteristicRange,
      public vulkan::pipeline::AddVertexShader,
#if !SEPARATE_FRAGMENT_SHADER
      public vulkan::pipeline::AddFragmentShader,
#endif
      public vulkan::pipeline::AddPushConstant
  {
   private:
    int m_pipeline;

    // Define pipeline objects.
    Square m_square;
    TopBottomPositions m_top_bottom_positions;

   protected:
    using direct_base_type = vulkan::pipeline::CharacteristicRange;

    // The different states of this task.
    enum VertexPipelineCharacteristicRange_state_type {
      VertexPipelineCharacteristicRange_initialize = direct_base_type::state_end,
      VertexPipelineCharacteristicRange_fill,
      VertexPipelineCharacteristicRange_preprocess,
      VertexPipelineCharacteristicRange_compile
    };

    ~VertexPipelineCharacteristicRange() override
    {
      DoutEntering(dc::vulkan, "VertexPipelineCharacteristicRange::~VertexPipelineCharacteristicRange() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = VertexPipelineCharacteristicRange_compile + 1;

    VertexPipelineCharacteristicRange(task::SynchronousWindow const* owning_window, int pipeline COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::CharacteristicRange(owning_window, 0, 2 COMMA_CWDEBUG_ONLY(debug)), m_pipeline(pipeline) { }

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
        AI_CASE_RETURN(VertexPipelineCharacteristicRange_preprocess);
        AI_CASE_RETURN(VertexPipelineCharacteristicRange_compile);
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

          //FIXME: do this in the Add* base classes
          // Register the vectors that we will fill.
//          m_flat_create_info->add(&m_vertex_input_binding_descriptions);
//          m_flat_create_info->add(&m_vertex_input_attribute_descriptions);
//          m_flat_create_info->add_descriptor_set_layouts(&sorted_descriptor_set_layouts());
//          m_flat_create_info->add(&m_push_constant_ranges);
//          m_flat_create_info->add(&m_shader_stage_create_infos);

          // Define the pipeline.
          add_push_constant<PushConstant>();
          add_vertex_input_binding(m_square);
          add_vertex_input_binding(m_top_bottom_positions);

//          m_vertex_input_binding_descriptions = vertex_binding_descriptions();

          set_continue_state(VertexPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_initialized;
          break;
        }
        case VertexPipelineCharacteristicRange_fill:
        {
          Dout(dc::notice, "fill_index = " << fill_index());
          set_continue_state(VertexPipelineCharacteristicRange_preprocess);
          run_state = CharacteristicRange_filled;
          break;
        }
        case VertexPipelineCharacteristicRange_preprocess:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Preprocess the shaders.
          {
            using namespace vulkan::shader_builder;

            ShaderIndex shader_vert_index = window->m_shader_indices[LocalShaderIndex::vertex0];

            // These two calls fill PipelineFactory::m_sorted_descriptor_set_layouts with arbitrary binding numbers (in the order that they are found in the shader template code).
            preprocess1(m_owning_window->application().get_shader_info(shader_vert_index));
          }
          // FIXME: do the below from the factory after all characteristic finished.
//          m_vertex_input_attribute_descriptions = vertex_input_attribute_descriptions();
//          m_push_constant_ranges = push_constant_ranges();

          // Generate vertex buffers.
          // FIXME: it seems weird to call this here, because create_vertex_buffers should only be called once
          // while the current function is part of a pipeline factory...
#if 0
          static std::atomic_int done = false;
          if (done.fetch_or(true) == false)
            window->create_vertex_buffers(this);
#endif

          // Realize the descriptor set layouts: if a layout already exists then use the existing
          // handle and update the binding values used in PipelineFactory::m_sorted_descriptor_set_layouts.
          // Otherwise, if it does not already exist, create a new descriptor set layout using the
          // provided binding values as-is.
          realize_descriptor_set_layouts(m_owning_window->logical_device());
          //
          set_continue_state(VertexPipelineCharacteristicRange_compile);
          run_state = CharacteristicRange_preprocessed;
          break;
        }
        case VertexPipelineCharacteristicRange_compile:
        {
          using namespace vulkan::shader_builder;
          Window const* window = static_cast<Window const*>(m_owning_window);

          ShaderIndex shader_vert_index = window->m_shader_indices[LocalShaderIndex::vertex0];

          // Compile the shaders.
          ShaderCompiler compiler;
          build_shader(m_owning_window, shader_vert_index, compiler, m_set_index_hint_map
              COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));

          set_continue_state(VertexPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_compiled;
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

#if SEPARATE_FRAGMENT_SHADER
  class FragmentPipelineCharacteristicRange : public vulkan::pipeline::CharacteristicRange,
      public vulkan::pipeline::AddFragmentShader,
      public vulkan::pipeline::AddPushConstant
  {
   private:
    int m_pipeline;

   protected:
    using direct_base_type = vulkan::pipeline::CharacteristicRange;

    // The different states of this task.
    enum FragmentPipelineCharacteristicRange_state_type {
      FragmentPipelineCharacteristicRange_initialize = direct_base_type::state_end,
      FragmentPipelineCharacteristicRange_fill,
      FragmentPipelineCharacteristicRange_preprocess,
      FragmentPipelineCharacteristicRange_compile
    };

    ~FragmentPipelineCharacteristicRange() override
    {
      DoutEntering(dc::vulkan, "FragmentPipelineCharacteristicRange::~FragmentPipelineCharacteristicRange() [" << this << "]");
    }

   public:
    static constexpr state_type state_end = FragmentPipelineCharacteristicRange_compile + 1;

    FragmentPipelineCharacteristicRange(task::SynchronousWindow const* owning_window, int pipeline COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::CharacteristicRange(owning_window, 0, 2 COMMA_CWDEBUG_ONLY(debug)), m_pipeline(pipeline) { }

   protected:
    char const* state_str_impl(state_type run_state) const override
    {
      switch(run_state)
      {
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_initialize);
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_fill);
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_preprocess);
        AI_CASE_RETURN(FragmentPipelineCharacteristicRange_compile);
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

          // Register the vectors that we will fill.
//          m_flat_create_info->add_descriptor_set_layouts(&sorted_descriptor_set_layouts());
//          m_flat_create_info->add(&m_push_constant_ranges);
//          m_flat_create_info->add(&m_shader_stage_create_infos);

          // Define the pipeline.
          add_push_constant<PushConstant>();

          set_continue_state(FragmentPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_initialized;
          break;
        }
        case FragmentPipelineCharacteristicRange_fill:
        {
          Dout(dc::notice, "fill_index = " << fill_index());
          Window const* window = static_cast<Window const*>(m_owning_window);

#if 0
          // Below we use 1 + m_pipeline as index into the array of combined image samplers.
          ASSERT(number_of_combined_image_samplers >= number_of_pipelines);
#if 0
          std::vector<vulkan::descriptor::SetKeyPreference> key_preference;
          for (int t = 0; t < number_of_combined_image_samplers; ++t)
            key_preference.emplace_back(window->combined_image_samplers()[t].descriptor_task()->descriptor_set_key(), 0.1);
#endif

          int constexpr number_of_combined_image_samplers_per_pipeline = 2;
          std::array<int, number_of_combined_image_samplers_per_pipeline> combined_image_sampler_indexes = {
            0,
            1 + m_pipeline
          };
          for (int i = 0; i < number_of_combined_image_samplers_per_pipeline; ++i)
          {
            add_combined_image_sampler(
                window->combined_image_samplers()[combined_image_sampler_indexes[i]]
#if 0
                , {}
                , { key_preference[combined_image_sampler_indexes[1 - i]] }
#endif
                );
          }
#else
          add_combined_image_sampler(window->combined_image_samplers()[0]);
#endif
          set_continue_state(FragmentPipelineCharacteristicRange_preprocess);
          run_state = CharacteristicRange_filled;
          break;
        }
        case FragmentPipelineCharacteristicRange_preprocess:
        {
          Window const* window = static_cast<Window const*>(m_owning_window);

          // Preprocess the shaders.
          {
            using namespace vulkan::shader_builder;

            ShaderIndex shader_frag_index = window->m_shader_indices[m_pipeline == 0 ? LocalShaderIndex::frag0 : LocalShaderIndex::frag1];

            // These two calls fill PipelineFactory::m_sorted_descriptor_set_layouts with arbitrary binding numbers (in the order that they are found in the shader template code).
            preprocess1(m_owning_window->application().get_shader_info(shader_frag_index));
          }
          // FIXME: do the below from the factory after all characteristics finished.
//          m_push_constant_ranges = push_constant_ranges();

          // Generate vertex buffers.
          // FIXME: it seems weird to call this here, because create_vertex_buffers should only be called once
          // while the current function is part of a pipeline factory...
          static std::atomic_int done = false;
          if (done.fetch_or(true) == false)
            window->create_vertex_buffers(this);

          // Realize the descriptor set layouts: if a layout already exists then use the existing
          // handle and update the binding values used in PipelineFactory::m_sorted_descriptor_set_layouts.
          // Otherwise, if it does not already exist, create a new descriptor set layout using the
          // provided binding values as-is.
          realize_descriptor_set_layouts(m_owning_window->logical_device());
          //
          set_continue_state(FragmentPipelineCharacteristicRange_compile);
          run_state = CharacteristicRange_preprocessed;
          break;
        }
        case FragmentPipelineCharacteristicRange_compile:
        {
          using namespace vulkan::shader_builder;
          Window const* window = static_cast<Window const*>(m_owning_window);

          ShaderIndex shader_frag_index = window->m_shader_indices[m_pipeline == 0 ? LocalShaderIndex::frag0 : LocalShaderIndex::frag1];

          // Compile the shaders.
          ShaderCompiler compiler;
          build_shader(m_owning_window, shader_frag_index, compiler, m_set_index_hint_map
              COMMA_CWDEBUG_ONLY({ m_owning_window, "PipelineFactory::m_shader_input_data" }));

          set_continue_state(FragmentPipelineCharacteristicRange_fill);
          run_state = CharacteristicRange_compiled;
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

  std::array<vulkan::pipeline::FactoryCharacteristicId, number_of_pipelines * number_of_characteristics> m_pipeline_factory_characteristic_range_ids;
  std::array<vulkan::pipeline::FactoryHandle, number_of_pipelines> m_pipeline_factory;

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");
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

    for (int pipeline = 0; pipeline < number_of_pipelines; ++pipeline)
    {
      m_pipeline_factory[pipeline] = create_pipeline_factory(m_graphics_pipelines[pipeline], main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
      // We have number_of_characteristics factory/range pair per pipeline here.
      for (int c = 0; c < number_of_characteristics; ++c)
      {
        vulkan::pipeline::FactoryCharacteristicId factory_characteristic_id;
        switch (c)
        {
          case 0:
            factory_characteristic_id =
              m_pipeline_factory[pipeline].add_characteristic<BasePipelineCharacteristic>(this, pipeline COMMA_CWDEBUG_ONLY(true));
            break;
          case 1:
            factory_characteristic_id =
              m_pipeline_factory[pipeline].add_characteristic<VertexPipelineCharacteristicRange>(this, pipeline COMMA_CWDEBUG_ONLY(true));
            break;
#if SEPARATE_FRAGMENT_SHADER
          case 2:
            factory_characteristic_id =
              m_pipeline_factory[pipeline].add_characteristic<FragmentPipelineCharacteristicRange>(this, pipeline COMMA_CWDEBUG_ONLY(true));
            break;
#endif
        }
        m_pipeline_factory_characteristic_range_ids[pipeline * number_of_characteristics + c] = factory_characteristic_id;
      }

      m_pipeline_factory[pipeline].generate(this);
    }
  }

  // This member function only uses m_vertex_buffers, which is aithreadsafe.
  // Therefore, the function as a whole is made threadsafe-const
  void create_vertex_buffers(vulkan::pipeline::CharacteristicRange const* pipeline_owner) /*threadsafe-*/ const
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers(" << pipeline_owner << ") [" << this << "]");

    for (vulkan::shader_builder::VertexShaderInputSetBase* vertex_shader_input_set : pipeline_owner->vertex_shader_input_sets())
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
bool have_all_pipelines = true;
for (int pl = 0; pl < number_of_pipelines; ++pl)
  if (!m_graphics_pipelines[pl].handle())
  {
    have_all_pipelines = false;
    break;
  }
if (!have_all_pipelines)
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
    m_imgui_stats_window.draw(io, m_imgui_timer);
  }
};
