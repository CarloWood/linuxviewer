#pragma once

#include "UniformBuffersTest.h"
#include "TopPosition.h"
#include "LeftPosition.h"
#include "BottomPosition.h"
#include <vulkan/VertexBuffers.h>
#include <vulkan/SynchronousWindow.h>
#include <vulkan/Pipeline.h>
#include <vulkan/pipeline/AddVertexShader.h>
#include <vulkan/pipeline/AddFragmentShader.h>
#include <vulkan/pipeline/Characteristic.h>
#include <vulkan/queues/CopyDataToImage.h>
#include <vulkan/shader_builder/ShaderIndex.h>
#include <vulkan/shader_builder/shader_resource/UniformBuffer.h>
#include <vulkan/shader_builder/shader_resource/CombinedImageSampler.h>
#include <vulkan/descriptor/SetKeyPreference.h>
#include <vulkan/vk_utils/ImageData.h>
#include <imgui.h>
#include "debug.h"
#include <vulkan/tracy/CwTracy.h>
#ifdef TRACY_ENABLE
#include <vulkan/tracy/SourceLocationDataIterator.h>
#endif

#define ENABLE_IMGUI 1

static constexpr int top_position_array_index = 6;

class Window : public vulkan::task::SynchronousWindow
{
 private:
  using Directory = vulkan::Directory;

 public:
  using vulkan::task::SynchronousWindow::SynchronousWindow;
  static constexpr condition_type sample_texture_uploaded = free_condition;

 private:
  // Define renderpass / attachment objects.
  RenderPass  main_pass{this, "main_pass"};
  Attachment      depth{this, "depth", s_depth_image_view_kind};

  vulkan::shader_builder::shader_resource::CombinedImageSampler m_combined_image_sampler;
  vulkan::Texture m_sample_texture;

  enum class LocalShaderIndex {
    vertex0,
    vertex1,
    frag0,
    frag1
  };
  utils::Array<vulkan::shader_builder::ShaderIndex, 4, LocalShaderIndex> m_shader_indices;

  vulkan::Pipeline m_graphics_pipeline0;
  vulkan::Pipeline m_graphics_pipeline1;

  vulkan::shader_resource::UniformBuffer<MyUniformBuffer> m_top_buffer{"m_top_buffer"};
  vulkan::shader_resource::UniformBuffer<LeftPosition> m_left_buffer{"m_left_buffer"};
  vulkan::shader_resource::UniformBuffer<BottomPosition> m_bottom_buffer{"m_bottom_buffer"};
  std::atomic_bool m_uniform_buffers_initialized = false;

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

  vulkan::FrameResourceIndex number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{10};
  }

  vulkan::SwapchainIndex number_of_swapchain_images() const override
  {
    return vulkan::SwapchainIndex{4};
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");

    // Sample texture.
    {
      vk_utils::stbi::ImageData texture_data(m_application->path_of(Directory::resources) / "textures/vort3_128x128.png", 4);

      // Create descriptor resources.
      static vulkan::ImageKind const sample_image_kind({
        .format = vk::Format::eR8G8B8A8Unorm,
        .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
      });

      static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

      m_sample_texture = vulkan::Texture(m_logical_device,
          texture_data.extent(), sample_image_view_kind,
          { .mipmapMode = vk::SamplerMipmapMode::eNearest,
            .anisotropyEnable = VK_FALSE },
          graphics_settings(),
          { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal }
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_sample_texture")));

      m_sample_texture.upload(texture_data.extent(), sample_image_view_kind, this,
          std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)), this, sample_texture_uploaded);
    }

    m_combined_image_sampler.update_image_sampler(&m_sample_texture, m_pipeline_factory_characteristic_id0);
    m_combined_image_sampler.update_image_sampler(&m_sample_texture, m_pipeline_factory_characteristic_id1);
  }

  void create_vertex_buffers() override
  {
    // We don't have vertex buffers.
  }

 private:
  static constexpr std::string_view uniform_buffer_controlled_triangle0_vert_glsl = R"glsl(
layout(location = 0) out vec2 v_Texcoord;

vec2 positions[3] = vec2[](
  vec2(0.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
);

void main()
{
  positions[0].x = MyUniformBuffer::foo[2].v[6].z - 1.0;
  positions[1].y = LeftPosition::y;
  positions[2].x = BottomPosition::x - 1.0;
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  v_Texcoord = 0.5 * (positions[gl_VertexIndex] + vec2(1.0, 1.0));
}
)glsl";

  static constexpr std::string_view uniform_buffer_controlled_triangle1_vert_glsl = R"glsl(
layout(location = 0) out vec2 v_Texcoord;

vec2 positions[3] = vec2[](
    vec2(0.0, -1.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

void main()
{
  positions[0].x = MyUniformBuffer::foo[2].v[6].z;
  positions[1].y = LeftPosition::y;
  positions[2].x = BottomPosition::x;
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  v_Texcoord = 0.5 * (positions[gl_VertexIndex] + vec2(1.0, 1.0));
}
)glsl";

  static constexpr std::string_view uniform_buffer_controlled_triangle0_frag_glsl = R"glsl(
layout(location = 0) in vec2 v_Texcoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 vort3_image = texture(CombinedImageSampler::vort3, v_Texcoord);
  outColor = vort3_image;
}
)glsl";

  static constexpr std::string_view uniform_buffer_controlled_triangle1_frag_glsl = R"glsl(
layout(location = 0) in vec2 v_Texcoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 vort3_image = texture(CombinedImageSampler::vort3, v_Texcoord);
  outColor = vort3_image;
}
)glsl";

  void register_shader_templates() override
  {
    DoutEntering(dc::notice, "Window::register_shader_templates() [" << this << "]");

    using namespace vulkan::shader_builder;

    // Create a ShaderInfo instance for each shader, initializing it with the stage that the shader will be used in and the template code that it exists of.
    std::vector<ShaderInfo> shader_info = {
      { vk::ShaderStageFlagBits::eVertex,   "uniform_buffer_controlled_triangle0.vert.glsl" },
      { vk::ShaderStageFlagBits::eVertex,   "uniform_buffer_controlled_triangle1.vert.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "uniform_buffer_controlled_triangle0.frag.glsl" },
      { vk::ShaderStageFlagBits::eFragment, "uniform_buffer_controlled_triangle1.frag.glsl" }
    };
    shader_info[0].load(uniform_buffer_controlled_triangle0_vert_glsl);
    shader_info[1].load(uniform_buffer_controlled_triangle1_vert_glsl);
    shader_info[2].load(uniform_buffer_controlled_triangle0_frag_glsl);
    shader_info[3].load(uniform_buffer_controlled_triangle1_frag_glsl);

    // Inform the application about the shaders that we use.
    // This will call hash() on each ShaderInfo object (which may only called once), and then store it only when that hash doesn't exist yet.
    // Since the hash is calculated over the template code (excluding generated declaration), the declarations are fixed for given template
    // code. The declarations determine the mapping between descriptor set, binding number, and shader variable.
    auto indices = application().register_shaders(std::move(shader_info));

    // Copy the returned "shader indices" into our local array.
    ASSERT(indices.size() == m_shader_indices.size());
    for (int i = 0; i < m_shader_indices.size(); ++i)
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

   protected:
    ~BasePipelineCharacteristic() override
    {
      DoutEntering(dc::vulkan, "BasePipelineCharacteristic::~BasePipelineCharacteristic() [" << this << "]");
    }

   public:
    BasePipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)) { }

   protected:
    void initialize() final
    {
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

  class UniformBuffersTestPipelineCharacteristic :
    public vulkan::pipeline::Characteristic,
    public vulkan::pipeline::AddVertexShader,
    public vulkan::pipeline::AddFragmentShader
  {
   private:
    int m_pipeline_factory;

   protected:
    ~UniformBuffersTestPipelineCharacteristic() override
    {
      DoutEntering(dc::vulkan, "UniformBuffersTestPipelineCharacteristic::~UniformBuffersTestPipelineCharacteristic() [" << this << "]");
    }

   public:
    UniformBuffersTestPipelineCharacteristic(vulkan::task::SynchronousWindow const* owning_window, int pipeline COMMA_CWDEBUG_ONLY(bool debug)) :
      vulkan::pipeline::Characteristic(owning_window COMMA_CWDEBUG_ONLY(debug)), m_pipeline_factory(pipeline)
    {
      // We do not have vertex buffers.
      m_use_vertex_buffers = false;
    }

   protected:
    // Overriden from vulkan::pipeline::CharacteristicBase.
    void initialize() final
    {
      Window const* window = static_cast<Window const*>(m_owning_window);

      // Define the pipeline.
      vulkan::descriptor::SetKeyPreference top_set_key_preference(window->m_top_buffer.descriptor_set_key(), 1.0);
      vulkan::descriptor::SetKeyPreference left_set_key_preference(window->m_left_buffer.descriptor_set_key(), 1.0);

      if (m_pipeline_factory == 0)
      {
        // This assigns top to descriptor set 0 and left to 1.
        add_uniform_buffer(window->m_top_buffer);
        add_uniform_buffer(window->m_left_buffer, {}, { top_set_key_preference });
      }
      else
      {
        // Swap top and left (hopefully resulting in that they swap descriptor sets; left in 0 and top in 1).
        add_uniform_buffer(window->m_left_buffer);
        add_uniform_buffer(window->m_top_buffer, {}, { left_set_key_preference });
      }
      // This assigns bottom to set 2.
      add_uniform_buffer(window->m_bottom_buffer, {}, { top_set_key_preference, left_set_key_preference });
      // The texture must go into the same set as top.
      add_combined_image_sampler(window->m_combined_image_sampler, { top_set_key_preference });

      vulkan::shader_builder::ShaderIndex vertex_shader_index =
        (m_pipeline_factory == 0) ? window->m_shader_indices[LocalShaderIndex::vertex0] : window->m_shader_indices[LocalShaderIndex::vertex1];
      vulkan::shader_builder::ShaderIndex fragment_shader_index =
        (m_pipeline_factory == 0) ? window->m_shader_indices[LocalShaderIndex::frag0] : window->m_shader_indices[LocalShaderIndex::frag1];
      add_shader(vertex_shader_index);
      add_shader(fragment_shader_index);
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (UniformBuffersTestPipelineCharacteristic*)" << this << " }";
    }
#endif
  };

  vulkan::pipeline::FactoryCharacteristicId m_pipeline_factory_characteristic_id0;
  vulkan::pipeline::FactoryCharacteristicId m_pipeline_factory_characteristic_id1;

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    m_combined_image_sampler.set_glsl_id_postfix("vort3");

    // This will become PipelineFactoryIndex #0, because it is created first.
    auto pipeline_factory0 = create_pipeline_factory(m_graphics_pipeline0, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory0.add_characteristic<BasePipelineCharacteristic>(this COMMA_CWDEBUG_ONLY(true));
    m_pipeline_factory_characteristic_id0 = pipeline_factory0.add_characteristic<UniformBuffersTestPipelineCharacteristic>(this, 0
        COMMA_CWDEBUG_ONLY(true));
    pipeline_factory0.generate(this);

    // This will become PipelineFactoryIndex #1.
    auto pipeline_factory1 = create_pipeline_factory(m_graphics_pipeline1, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    pipeline_factory1.add_characteristic<BasePipelineCharacteristic>(this COMMA_CWDEBUG_ONLY(true));
    m_pipeline_factory_characteristic_id1 = pipeline_factory1.add_characteristic<UniformBuffersTestPipelineCharacteristic>(this, 1
        COMMA_CWDEBUG_ONLY(true));
    pipeline_factory1.generate(this);
  }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval frame_rate_interval() const override
  {
    DoutEntering(dc::vulkan, "Window::frame_rate_interval() [" << this << "]");
    // Limit the frame rate of this window to 1000 frames per second.
    return threadpool::Interval<100, std::chrono::milliseconds>{};
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
    command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass1, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass2, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          number_of_swapchain_images(), swapchain_index);

      command_buffer.beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
if (!m_graphics_pipeline0.handle() || !m_graphics_pipeline1.handle())
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer.setViewport(0, { viewport });
      command_buffer.setScissor(0, { scissor });

      command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline0.handle()));
      command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline0.layout(), 0 /* uint32_t first_set */,
          m_graphics_pipeline0.vhv_descriptor_sets(m_current_frame.m_resource_index), {});

      command_buffer.draw(3, 1, 0, 0);

      command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline1.handle()));
      command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline1.layout(), 0 /* uint32_t first_set */,
          m_graphics_pipeline1.vhv_descriptor_sets(m_current_frame.m_resource_index), {});

      command_buffer.draw(3, 1, 0, 0);
}
      command_buffer.endRenderPass();
      TracyVkCollect(presentation_surface().tracy_context(), static_cast<vk::CommandBuffer>(command_buffer));
    }
#if ENABLE_IMGUI
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass1, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __imgui_pass2, static_cast<vk::CommandBuffer>(command_buffer), imgui_pass.name(), true,
          number_of_swapchain_images(), swapchain_index);
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

  UniformBuffersTest const& application() const
  {
    return static_cast<UniformBuffersTest const&>(vulkan::task::SynchronousWindow::application());
  }

  float m_top_position{0.5};
  float m_left_position{0};
  float m_bottom_position{0.5};

  void draw_imgui() override final
  {
    ZoneScopedN("Window::draw_imgui");
    DoutEntering(dc::vkframe, "Window::draw_imgui() [" << this << "]");

    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_imgui_timer);

    //ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f));
    ImGui::Begin(reinterpret_cast<char const*>(application().application_name().c_str()), nullptr, ImGuiWindowFlags_None);
    ImGui::SliderFloat("Top position", &m_top_position, 0.0, 1.0);
    ImGui::SliderFloat("Left position", &m_left_position, -1.0, 1.0);
    ImGui::SliderFloat("Bottom position", &m_bottom_position, 0.0, 1.0);
    ImGui::End();

    vulkan::FrameResourceIndex frame_index = m_current_frame.m_resource_index;
    if (m_top_buffer.is_created())
      ((MyUniformBuffer*)(m_top_buffer[frame_index].pointer()))->foo[2].v[top_position_array_index][2] = m_top_position + frame_index.get_value() * 0.1f;
    if (m_left_buffer.is_created())
      ((LeftPosition*)(m_left_buffer[frame_index].pointer()))->y = m_left_position;
    if (m_bottom_buffer.is_created())
      ((BottomPosition*)(m_bottom_buffer[frame_index].pointer()))->x = m_bottom_position;
  }
};
