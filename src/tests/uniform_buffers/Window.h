#pragma once

#include "UniformBuffersTest.h"
#include "SynchronousWindow.h"
#include "TopPosition.h"
#include "LeftPosition.h"
#include "BottomPosition.h"
#include "UniformBufferSlider.h"
#include "queues/CopyDataToImage.h"
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

#define ENABLE_IMGUI 1

static constexpr int top_position_array_size = 32;
static constexpr int top_position_array_index = 27;

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

  vulkan::shader_resource::Texture m_sample_texture{"m_sample_texture"};

  enum class LocalShaderIndex {
    vertex0,
    vertex1,
    frag0,
    frag1
  };
  utils::Array<vulkan::shader_builder::ShaderIndex, 4, LocalShaderIndex> m_shader_indices;

  vulkan::Pipeline m_graphics_pipeline0;
  vulkan::Pipeline m_graphics_pipeline1;

  UniformBufferSlider<TopPosition> m_top_buffer{"m_top_buffer"};
  UniformBufferSlider<LeftPosition> m_left_buffer{"m_left_buffer"};
  UniformBufferSlider<BottomPosition> m_bottom_buffer{"m_bottom_buffer"};
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

  vulkan::FrameResourceIndex max_number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{1};
  }

  vulkan::SwapchainIndex max_number_of_swapchain_images() const override
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
      {
        static vulkan::ImageKind const sample_image_kind({
          .format = vk::Format::eR8G8B8A8Unorm,
          .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
        });

        static vulkan::ImageViewKind const sample_image_view_kind(sample_image_kind, {});

        m_sample_texture = vulkan::shader_resource::Texture("vort3", m_logical_device,
            texture_data.extent(), sample_image_view_kind,
            { .mipmapMode = vk::SamplerMipmapMode::eNearest,
              .anisotropyEnable = VK_FALSE },
            graphics_settings(),
            { .properties = vk::MemoryPropertyFlagBits::eDeviceLocal });

        auto copy_data_to_image = statefultask::create<task::CopyDataToImage>(m_logical_device, texture_data.size(),
            m_sample_texture.m_vh_image, texture_data.extent(), vk_defaults::ImageSubresourceRange{},
            vk::ImageLayout::eUndefined, vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
            COMMA_CWDEBUG_ONLY(true));

        copy_data_to_image->set_resource_owner(this);   // Wait for this task to finish before destroying this window, because this window owns the texture (m_sample_texture).
        copy_data_to_image->set_data_feeder(std::make_unique<vk_utils::stbi::ImageDataFeeder>(std::move(texture_data)));
        copy_data_to_image->run(vulkan::Application::instance().low_priority_queue());
      }
    }
  }

 public:
#if 0
  void create_uniform_buffers(vulkan::pipeline::ShaderInputData const& shader_input_data,
      vulkan::descriptor::SetBindingMap const& set_binding_map) override
  {
    DoutEntering(dc::shaderresource|dc::vulkan, "Window::create_uniform_buffers(...) [" << this << "]");
    utils::Vector<vk::DescriptorSetLayout, vulkan::descriptor::SetIndex> vhv_descriptor_set_layouts = shader_input_data.get_vhv_descriptor_set_layouts(set_binding_map);

    vulkan::descriptor::SetIndex top_descriptor_set_index = set_binding_map.convert(shader_input_data.get_set_index_hint(m_top_buffer.descriptor_set_key()));
    vulkan::descriptor::SetIndex left_descriptor_set_index = set_binding_map.convert(shader_input_data.get_set_index_hint(m_left_buffer.descriptor_set_key()));
    vulkan::descriptor::SetIndex bottom_descriptor_set_index = set_binding_map.convert(shader_input_data.get_set_index_hint(m_bottom_buffer.descriptor_set_key()));

    //FIXME: Find a way to find the actual texture_binding.
    uint32_t texture_binding = 1;

    // The descriptor set layouts are the same for both pipelines in this application, and because the descriptor sets
    // created from it are updated with the same state, we also reuse the descriptors sets and have both pipelines use
    // the same descriptor sets (this function is only called once: for whichever UniformBuffersTestPipelineCharacteristicBase's
    // initialize finished first).
    m_vhv_descriptor_sets = logical_device()->allocate_descriptor_sets(
        vhv_descriptor_set_layouts,
        logical_device()->get_descriptor_pool()
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_vhv_descriptor_set")));

    m_top_buffer.create(this, top_position_array_size * sizeof(TopPosition)
        COMMA_CWDEBUG_ONLY(debug_name_prefix("Window::m_top_buffer")));
    m_left_buffer.create(this, sizeof(LeftPosition)
        COMMA_CWDEBUG_ONLY(debug_name_prefix("Window::m_left_buffer")));
    m_bottom_buffer.create(this, sizeof(BottomPosition)
        COMMA_CWDEBUG_ONLY(debug_name_prefix("Window::m_bottom_buffer")));

    // FIXME: not implemented: currently we're only using the first frame resource!
    vulkan::FrameResourceIndex const hack0{0};

    // Information about the buffer we want to point at in the descriptor.
    std::vector<vk::DescriptorBufferInfo> top_buffer_infos = {
      {
        .buffer = m_top_buffer[hack0].m_vh_buffer,
        .offset = 0,
        .range = top_position_array_size * sizeof(TopPosition)
      }
    };
    std::vector<vk::DescriptorBufferInfo> left_buffer_infos = {
      {
        .buffer = m_left_buffer[hack0].m_vh_buffer,
        .offset = 0,
        .range = sizeof(LeftPosition)
      }
    };
    std::vector<vk::DescriptorBufferInfo> bottom_buffer_infos = {
      {
        .buffer = m_bottom_buffer[hack0].m_vh_buffer,
        .offset = 0,
        .range = sizeof(BottomPosition)
      }
    };

    //vkUpdateDescriptorSets(_device, 1, &setWrite, 0, nullptr);
    uint32_t binding = 0;
    logical_device()->update_descriptor_set(m_vhv_descriptor_sets[top_descriptor_set_index], vk::DescriptorType::eUniformBuffer, 1 - texture_binding, 0, {}, top_buffer_infos);
    logical_device()->update_descriptor_set(m_vhv_descriptor_sets[left_descriptor_set_index], vk::DescriptorType::eUniformBuffer, binding, 0, {}, left_buffer_infos);
    logical_device()->update_descriptor_set(m_vhv_descriptor_sets[bottom_descriptor_set_index], vk::DescriptorType::eUniformBuffer, binding, 0, {}, bottom_buffer_infos);

    // Fill the buffer...
    for (int i = 0; i < top_position_array_size; ++i)
      ((TopPosition*)(m_top_buffer[hack0].pointer()))[i].x = 0.8;
    ((LeftPosition*)(m_left_buffer[hack0].pointer()))->y = 0.5;
    ((BottomPosition*)(m_bottom_buffer[hack0].pointer()))->x = 0.5;

    // Tell the render loop that we're done initializing the uniform buffers.
    m_uniform_buffers_initialized = true;

    // Update descriptor set of m_sample_texture.
    {
      std::vector<vk::DescriptorImageInfo> image_infos = {
        {
          .sampler = m_sample_texture.sampler(),
          .imageView = m_sample_texture.image_view(),
          .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        }
      };
      // The 'top' descriptor set also contains the texture.
      m_logical_device->update_descriptor_set(m_vhv_descriptor_sets[top_descriptor_set_index], vk::DescriptorType::eCombinedImageSampler, texture_binding, 0, image_infos);
    }
  }
#endif

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
  positions[0].x = TopPosition::x - 1.0;
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
  positions[0].x = TopPosition::x;
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
  vec4 vort3_image = texture(Texture::vort3, v_Texcoord);
  outColor = vort3_image;
}
)glsl";

  static constexpr std::string_view uniform_buffer_controlled_triangle1_frag_glsl = R"glsl(
layout(location = 0) in vec2 v_Texcoord;

layout(location = 0) out vec4 outColor;

void main()
{
  vec4 vort3_image = texture(Texture::vort3, v_Texcoord);
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
    // code. The declaration determine the mapping between descriptor set, and binding number, and shader variable.
    auto indices = application().register_shaders(std::move(shader_info));

    // Copy the returned "shader indices" into our local array.
    ASSERT(indices.size() == m_shader_indices.size());
    for (int i = 0; i < 4; ++i)
      m_shader_indices[static_cast<LocalShaderIndex>(i)] = indices[i];
  }

  class UniformBuffersTestPipelineCharacteristicBase : public vulkan::pipeline::Characteristic
  {
   private:
    std::vector<vk::PipelineColorBlendAttachmentState> m_pipeline_color_blend_attachment_states;
    std::vector<vk::DynamicState> m_dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };
#if 0
    std::vector<vulkan::descriptor::SetLayout> m_descriptor_set_layouts;

    vulkan::descriptor::SetLayout m_top_descriptor_set_layout;
    vulkan::descriptor::SetLayout m_left_descriptor_set_layout;
    vulkan::descriptor::SetLayout m_bottom_descriptor_set_layout;
#endif

   protected:
    void initializeX(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow const* owning_window, int pipeline)
    {
      DoutEntering(dc::vulkan, "initializeX(FlatCreateInfo @" << (void*)&flat_create_info << ", " << owning_window << ", " << pipeline << ")");

      Window const* window = static_cast<Window const*>(owning_window);

      // Register the vectors that we will fill.
      flat_create_info.add(&shader_input_data().shader_stage_create_infos());
      flat_create_info.add(&m_pipeline_color_blend_attachment_states);
      flat_create_info.add(&m_dynamic_states);
      flat_create_info.add_descriptor_set_layouts(&shader_input_data().sorted_descriptor_set_layouts());

      // Define the pipeline.
      vulkan::descriptor::SetKeyPreference top_set_key_preference(window->m_top_buffer.descriptor_set_key(), 1.0);
      vulkan::descriptor::SetKeyPreference left_set_key_preference(window->m_left_buffer.descriptor_set_key(), 1.0);

      if (pipeline == 0)
      {
        // This assigns top to descriptor set 0 and left to 1.
        shader_input_data().add_uniform_buffer(window->m_top_buffer);
        shader_input_data().add_uniform_buffer(window->m_left_buffer, {}, { top_set_key_preference });
      }
      else
      {
        // Swap top and left (hopefully resulting in that they swap descriptor sets; left in 0 and top in 1).
        shader_input_data().add_uniform_buffer(window->m_left_buffer);
        shader_input_data().add_uniform_buffer(window->m_top_buffer, {}, { left_set_key_preference });
      }
      // This assigns bottom to set 2.
      shader_input_data().add_uniform_buffer(window->m_bottom_buffer, {}, { top_set_key_preference, left_set_key_preference });
      // The texture must go into the same set as top.
      shader_input_data().add_texture(window->m_sample_texture, { top_set_key_preference });

      // Add default color blend.
      m_pipeline_color_blend_attachment_states.push_back(vk_defaults::PipelineColorBlendAttachmentState{});

      // Compile the shaders.
      {
        using namespace vulkan::shader_builder;

        ShaderIndex shader_vert_index = (pipeline == 0) ? window->m_shader_indices[LocalShaderIndex::vertex0] : window->m_shader_indices[LocalShaderIndex::vertex1];
        ShaderIndex shader_frag_index = (pipeline == 0) ? window->m_shader_indices[LocalShaderIndex::frag0] : window->m_shader_indices[LocalShaderIndex::frag1];

        //ShaderCompiler compiler;

        //shader_input_data().build_shader(owning_window, shader_vert_index, compiler
        //shader_input_data().build_shader(owning_window, shader_frag_index, compiler

        // These two calls fill ShaderInputData::m_sorted_descriptor_set_layouts with arbitrary binding numbers (in the order that they are found in the shader template code).
        shader_input_data().preprocess1(owning_window->application().get_shader_info(shader_vert_index));
        shader_input_data().preprocess1(owning_window->application().get_shader_info(shader_frag_index));

        // Compile the shaders.
        flat_create_info.add_set_binding_map_callback(
            [=, this](vulkan::descriptor::SetBindingMap const& set_binding_map)
            {
              Dout(dc::vulkan, "Calling set_binding_callback lambda with " << set_binding_map << " [" << this << "]");
              ShaderCompiler compiler;

              shader_input_data().build_shader(owning_window, shader_vert_index, compiler, set_binding_map
                  COMMA_CWDEBUG_ONLY({ owning_window, "PipelineFactory::m_shader_input_data" }));
              shader_input_data().build_shader(owning_window, shader_frag_index, compiler, set_binding_map
                  COMMA_CWDEBUG_ONLY({ owning_window, "PipelineFactory::m_shader_input_data" }));
            });
      }

      flat_create_info.m_pipeline_input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

#if 0
      //FIXME: these vectors should be generated.
      {
        std::vector<vk::DescriptorSetLayoutBinding> top_layout_bindings = {
          {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
          },
          {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
          }
        };
        std::vector<vk::DescriptorSetLayoutBinding> left_layout_bindings = {
          {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
          }
        };
        std::vector<vk::DescriptorSetLayoutBinding> bottom_layout_bindings = {
          {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
          }
        };

        m_top_descriptor_set_layout = owning_window->logical_device()->realize_descriptor_set_layout(std::move(top_layout_bindings));
        m_left_descriptor_set_layout = owning_window->logical_device()->realize_descriptor_set_layout(std::move(left_layout_bindings));
        m_bottom_descriptor_set_layout = owning_window->logical_device()->realize_descriptor_set_layout(std::move(bottom_layout_bindings));
      }

      if (pipeline == 0)
      {
        // Define pipeline layout.
        m_descriptor_set_layouts.push_back(m_top_descriptor_set_layout);
        m_descriptor_set_layouts.push_back(m_left_descriptor_set_layout);
        m_descriptor_set_layouts.push_back(m_bottom_descriptor_set_layout);
      }
      else
      {
        // Define pipeline layout.
        m_descriptor_set_layouts.push_back(m_left_descriptor_set_layout);
        m_descriptor_set_layouts.push_back(m_top_descriptor_set_layout);
        m_descriptor_set_layouts.push_back(m_bottom_descriptor_set_layout);
      }
#endif

      // Realize the descriptor set layouts: if a layout already exists then use the existing
      // handle and update the binding values used in ShaderInputData::m_sorted_descriptor_set_layouts.
      // Otherwise, if it does not already exist, create a new descriptor set layout using the
      // provided binding values as-is.
      shader_input_data().realize_descriptor_set_layouts(owning_window->logical_device());
    }

   public:
#ifdef CWDEBUG
    void print_on(std::ostream& os) const override
    {
      os << "{ (UniformBuffersTestPipelineCharacteristicBase*)" << this << " }";
    }
#endif
  };

  class UniformBuffersTestPipelineCharacteristic0 : public UniformBuffersTestPipelineCharacteristicBase
  {
    void initialize(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow const* owning_window) override
    {
      initializeX(flat_create_info, owning_window, 0);
    }
  };

  class UniformBuffersTestPipelineCharacteristic1 : public UniformBuffersTestPipelineCharacteristicBase
  {
    void initialize(vulkan::pipeline::FlatCreateInfo& flat_create_info, task::SynchronousWindow const* owning_window) override
    {
      initializeX(flat_create_info, owning_window, 1);
    }
  };

  vulkan::pipeline::FactoryHandle m_pipeline_factory0;  // This will become PipelineFactoryIndex #0, because it is created first.
  vulkan::pipeline::FactoryHandle m_pipeline_factory1;  // This will become PipelineFactoryIndex #1.
  boost::intrusive_ptr<task::PipelineFactory> m_pipeline_factory0_keep_alive;   // Keep alive so we can access m_pipeline_factory0 later.

  void create_graphics_pipelines() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipelines() [" << this << "]");

    m_pipeline_factory0 = create_pipeline_factory(m_graphics_pipeline0, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    m_pipeline_factory0_keep_alive = pipeline_factory(m_pipeline_factory0.factory_index());
    m_pipeline_factory0.add_characteristic<UniformBuffersTestPipelineCharacteristic0>(this);
    m_pipeline_factory0.generate(this);

    m_pipeline_factory1 = create_pipeline_factory(m_graphics_pipeline1, main_pass.vh_render_pass() COMMA_CWDEBUG_ONLY(true));
    m_pipeline_factory1.add_characteristic<UniformBuffersTestPipelineCharacteristic1>(this);
    m_pipeline_factory1.generate(this);
  }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    DoutEntering(dc::vulkan, "Window::get_frame_rate_interval() [" << this << "]");
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
    command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
    {
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass1, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_frame_resources(), m_current_frame.m_resource_index);
      CwTracyVkNamedZone(presentation_surface().tracy_context(), __main_pass2, static_cast<vk::CommandBuffer>(command_buffer), main_pass.name(), true,
          max_number_of_swapchain_images(), swapchain_index);

      command_buffer->beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
// FIXME: this is a hack - what we really need is a vector with RenderProxy objects.
if (!m_graphics_pipeline0.handle() || !m_graphics_pipeline1.handle())
  Dout(dc::warning, "Pipeline not available");
else
{
      command_buffer->setViewport(0, { viewport });
      command_buffer->setScissor(0, { scissor });

      command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline0.handle()));
      command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline0.layout(), 0 /* uint32_t first_set */,
          m_graphics_pipeline0.vhv_descriptor_sets(), {});

      command_buffer->draw(3, 1, 0, 0);

      command_buffer->bindPipeline(vk::PipelineBindPoint::eGraphics, vh_graphics_pipeline(m_graphics_pipeline1.handle()));
      command_buffer->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline1.layout(), 0 /* uint32_t first_set */,
          m_graphics_pipeline1.vhv_descriptor_sets(), {});

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

  float m_top_position{0.5};
  float m_left_position{0};
  float m_bottom_position{0.5};

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
    ImGui::SliderFloat("Left position", &m_left_position, -1.0, 1.0);
    ImGui::SliderFloat("Bottom position", &m_bottom_position, 0.0, 1.0);
    ImGui::End();

    // FIXME: not implemented: currently we're only using the first frame resource!
    vulkan::FrameResourceIndex const hack0{0};

    if (m_top_buffer.is_ready())
      ((TopPosition*)(m_top_buffer[hack0].pointer()))[0].x = m_top_position;
    if (m_left_buffer.is_ready())
      ((LeftPosition*)(m_left_buffer[hack0].pointer()))->y = m_left_position;
    if (m_bottom_buffer.is_ready())
      ((BottomPosition*)(m_bottom_buffer[hack0].pointer()))->x = m_bottom_position;
  }
};
