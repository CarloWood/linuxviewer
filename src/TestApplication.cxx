#include "sys.h"
#include "TestApplication.h"
#include "SampleParameters.h"
#include "VertexData.h"
#include "InstanceData.h"
#include "vulkan/FrameResourcesData.h"
#include "vulkan/LogicalDevice.h"
#include "vulkan/infos/DeviceCreateInfo.h"
#include "vulkan/rendergraph/Attachment.h"
#include "vulkan/rendergraph/RenderPass.h"
#include "vulkan/Pipeline.h"
#include "vulkan/pipeline/Pipeline.h"
#include "vulkan/pipeline/VertexShaderInputSet.h"
#include "vk_utils/get_image_data.h"
#include "debug.h"
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif
#include "shaderbuilder/ShaderModule.h"

#include "vulkan/rendergraph/ClearValue.h"
#include "vulkan/ImageKind.h"
#include <imgui.h>
#include <Eigen/Geometry>
#include <random>

#define ADD_STATS_TO_SINGLE_BUTTON_WINDOW 0
#define ENABLE_IMGUI 1

using namespace linuxviewer;

class SingleButtonWindow : public task::SynchronousWindow
{
  std::function<void(SingleButtonWindow&)> m_callback;

#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
  imgui::StatsWindow m_imgui_stats_window;
#endif

 public:
  SingleButtonWindow(std::function<void(SingleButtonWindow&)> callback, vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug)) :
    task::SynchronousWindow(application COMMA_CWDEBUG_ONLY(debug)), m_callback(callback) { }

 private:
  void create_render_graph() override
  {
    DoutEntering(dc::vulkan, "SingleButtonWindow::create_render_graph() [" << this << "]");

    // This must be a reference.
    auto& output = swapchain().presentation_attachment();

    // This window draws nothing but an ImGui window.
    m_render_graph = imgui_pass->stores(~output);

    // Generate everything.
    m_render_graph.generate(this);
  }

  void create_descriptor_set() override { }
  void create_textures() override { }
  void create_pipeline_layout() override { }
  void create_graphics_pipeline() override { }
  void create_vertex_buffers() override { }

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 11.111 frames per second.
    return threadpool::Interval<90, std::chrono::milliseconds>{};
  }

  //===========================================================================
  //
  // Frame code (called every frame)
  //
  //===========================================================================

  void draw_frame() override
  {
    DoutEntering(dc::vkframe, "SingleButtonWindow::draw_frame() [" << this << "]");

    start_frame();
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;
    imgui_pass.update_image_views(swapchain(), frame_resources);

    m_logical_device->reset_fences({ *frame_resources->m_command_buffers_completed });
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_command_buffer(command_pool_w);

      Dout(dc::vkframe, "Start recording command buffer.");
      command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
      command_buffer_w->beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
      m_imgui.render_frame(command_buffer_w, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
      command_buffer_w->endRenderPass();
      command_buffer_w->end();
      Dout(dc::vkframe, "End recording command buffer.");

      submit(command_buffer_w);
    } // Unlock command pool.

    // Draw GUI and present swapchain image.
    finish_frame();
  }

  //===========================================================================
  //
  // ImGui
  //
  //===========================================================================

  void draw_imgui() override final
  {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("SingleButton", nullptr,
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings);

    if (ImGui::Button("Trigger Event", ImVec2(150.f - 16.f, 50.f - 16.f)))
    {
      Dout(dc::notice, "SingleButtonWindow: calling m_callback() [" << this << "]");
      m_callback(*this);
    }

    ImGui::End();

#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 120.0f, 20.0f));
    m_imgui_stats_window.draw(io, m_timer);
#endif
  }
};

class HeavyRectangle final : public vulkan::pipeline::VertexShaderInputSet<VertexData>
{
  using Vector2f = Eigen::Vector2f;
  using Transform = Eigen::Affine2f;

  // Each batch exists of two triangles that form a square with coordinates (pos_x, pos_y) where
  // both pos_x and pos_y run from -size to +size in SampleParameters::s_quad_tessellation steps.
  //
  //                                  __ iside = 4
  //                                 /
  //                                v
  //    y=0 ┬─────┬─────┬─────┬─────┐
  //        │     │     │     │     │
  //        │     │     │     │     │  x runs from 0..iside-1
  //      1 ┼─────A─────B─────┼─────┤    y runs from 0..iside-1
  //        │     │   .🮠│     │     │       first triangle: A C B
  //        │     │🮣    │     │     │      second triangle: B C D, both counter clockwise.
  //      2 ┼─────C─────D─────┼─────┤
  //        │     │     │     │     │  Shown are the triangles that belong to x,y = 1,1 (A).
  //        │     │     │     │     │
  //      3 ┼─────┼─────┼─────┼─────┤
  //        │     │     │     │     │
  //        │     │     │     │     │
  //        ├─────┼─────┼─────┼─────┘
  //      x=0     1     2     3
  //        |                       |
  // pos: -size         0          +size
  //  uv:   0                       1
  //
  // At the same time (u, v) correspond with the texture coordinates and run from (0, 0) till (1, 1).
  //
  // Internally we use integer coordinates x and y that run from 0 till SampleParameters::s_quad_tessellation.
  //
  static constexpr int iside       = SampleParameters::s_quad_tessellation;
  static constexpr float size      = 0.12f;
  static constexpr float pos_scale = 2 * size / iside;
  static constexpr float uv_scale  = 1.0f / iside;
  static constexpr int batch_size  = 6;

  static Transform const xy_to_position;
  static Transform const xy_to_uv;
  static Vector2f const offset[6];

  Vector2f xy = {};             // Integer coordinates x,y.

  // Convert xy to one of the six corners of the two triangles.
  auto position_at(int vertex)
  {
    glsl::vec4 position;
    position << xy_to_position * (xy + offset[vertex]), 0.0f, 1.0f;     // Homogeneous coordinates.
    return position;
  }

  auto texture_coordinates_at(int vertex)
  {
    glsl::vec2 coords;
    coords << xy_to_uv * (xy + offset[vertex]);
    return coords;
  }

  int count() const override
  {
    return batch_size * iside * iside;
  }

  int next_batch() const override
  {
    return batch_size;
  }

  void create_entry(VertexData* input_entry_ptr) override
  {
    for (int vertex = 0; vertex < batch_size; ++vertex)
    {
      input_entry_ptr[vertex].m_position = position_at(vertex);
      input_entry_ptr[vertex].m_texture_coordinates = texture_coordinates_at(vertex);
    }

    // Advance to the next square.
    if (++xy.x() == iside) { xy.x() = 0; ++xy.y(); }
  }
};

// static transformation matrices.
HeavyRectangle::Transform const HeavyRectangle::xy_to_position = Transform(Eigen::Translation2f{-size, size}).scale(pos_scale);
HeavyRectangle::Transform const HeavyRectangle::xy_to_uv       = Transform{Transform::Identity()}.scale(uv_scale);
HeavyRectangle::Vector2f const HeavyRectangle::offset[batch_size] = {
  { 0.0f, 0.0f },   // A
  { 0.0f, 1.0f },   // C
  { 1.0f, 0.0f },   // B
  { 1.0f, 0.0f },   // B
  { 0.0f, 1.0f },   // C
  { 1.0f, 1.0f },   // D
};

class RandomPositions final : public vulkan::pipeline::VertexShaderInputSet<InstanceData>
{
  std::random_device m_random_device;
  std::mt19937 m_generator;
  std::uniform_real_distribution<float> m_distribution_xy;
  std::uniform_real_distribution<float> m_distribution_z;

 public:
  RandomPositions() : m_generator(m_random_device()), m_distribution_xy(-1.0f, 1.0f), m_distribution_z(0.0f, 1.0f) { }

 private:
  int count() const override
  {
    return SampleParameters::s_max_object_count;
  }

  void create_entry(InstanceData* input_entry_ptr) override final
  {
    input_entry_ptr->m_position << m_distribution_xy(m_generator), m_distribution_xy(m_generator), m_distribution_z(m_generator), 1.0f; // Homogeneous coordinates.
  }
};

class Window : public task::SynchronousWindow
{
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

  // Define pipeline objects.
  vulkan::pipeline::Pipeline m_pipeline;
  HeavyRectangle m_heavy_rectangle;             // A rectangle with many vertices.
  RandomPositions m_random_positions;           // Where to put those rectangles.

  vk::UniquePipeline m_graphics_pipeline;
  vulkan::BufferParameters m_vertex_buffer;
  vulkan::BufferParameters m_instance_buffer;
  vulkan::Texture m_background_texture;
  vulkan::Texture m_texture;
  vk::UniquePipelineLayout m_pipeline_layout;

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

    m_descriptor_set = logical_device().create_descriptor_resources(layout_bindings, pool_sizes
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

    // Define the pipeline.
    m_pipeline.bind(m_heavy_rectangle);
    m_pipeline.bind(m_random_positions);

    vulkan::Pipeline pipeline(this);

    {
      using namespace vulkan::shaderbuilder;

      ShaderCompiler compiler;
      ShaderCompilerOptions options;
      LocationContext location_context;

      ShaderModule shader_vert(vk::ShaderStageFlagBits::eVertex);
      shader_vert.set_name("intel.vert.glsl").load(intel_vert_glsl, location_context).compile(compiler, options);
      pipeline.add(shader_vert COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::pipeline")));

      ShaderModule shader_frag(vk::ShaderStageFlagBits::eFragment);
      shader_frag.set_name("intel.frag.glsl").load(intel_frag_glsl, location_context).compile(compiler, options);
      pipeline.add(shader_frag COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::pipeline")));
    }

    std::vector<vk::VertexInputBindingDescription> vertex_binding_description = {
      {
        .binding = 0,
        .stride = sizeof(VertexData),
        .inputRate = vk::VertexInputRate::eVertex
      },
      {
        .binding = 1,
        .stride = sizeof(InstanceData),
        .inputRate = vk::VertexInputRate::eInstance
      }
    };
    std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
      {
        .location = 0,
        .binding = vertex_binding_description[0].binding,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = offsetof(VertexData, m_position)
      },
      {
        .location = 1,
        .binding = vertex_binding_description[0].binding,
        .format = vk::Format::eR32G32Sfloat,
        .offset = offsetof(VertexData, m_texture_coordinates)
      },
      {
        .location = 2,
        .binding = vertex_binding_description[1].binding,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = offsetof(InstanceData, m_position)
      }
    };

    //=========================================================================
    // Vertex input.

    vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size()),
      .pVertexBindingDescriptions = vertex_binding_description.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
      .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
      .topology = vk::PrimitiveTopology::eTriangleList
    };

    vk::PipelineViewportStateCreateInfo viewport_state_create_info{
      .viewportCount = 1,
      .pViewports = nullptr,
      .scissorCount = 1,
      .pScissors = nullptr
    };
    vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
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
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
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
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()
    };

    auto const& shader_stage_create_infos = pipeline.shader_stage_create_infos();

    vk::GraphicsPipelineCreateInfo pipeline_create_info{
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

    // Generate everything.
    m_pipeline.generate(this
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline")));
  }

  void create_vertex_buffers() override
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers() [" << this << "]");

    // 3D model
    std::vector<VertexData> vertex_data;

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

    m_vertex_buffer = logical_device().create_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(VertexData),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_vertex_buffer")));
    copy_data_to_buffer(static_cast<uint32_t>(vertex_data.size()) * sizeof(VertexData), vertex_data.data(), *m_vertex_buffer.m_buffer, 0, vk::AccessFlags(0),
        vk::PipelineStageFlagBits::eTopOfPipe, vk::AccessFlagBits::eVertexAttributeRead, vk::PipelineStageFlagBits::eVertexInput);

    // Per instance data (position offsets and distance)
    std::vector<float> instance_data(SampleParameters::s_max_object_count * 4);
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
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_command_buffer(command_pool_w);

      Dout(dc::vkframe, "Start recording command buffer.");
      command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
      command_buffer_w->beginRenderPass(main_pass.begin_info(), vk::SubpassContents::eInline);
      command_buffer_w->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
      command_buffer_w->bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { *m_descriptor_set.m_handle }, {});
      command_buffer_w->bindVertexBuffers(0, { *m_vertex_buffer.m_buffer, *m_instance_buffer.m_buffer }, { 0, 0 });
      command_buffer_w->setViewport(0, { viewport });
      command_buffer_w->pushConstants(*m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ), &scaling_factor);
      command_buffer_w->setScissor(0, { scissor });
      command_buffer_w->draw(6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, m_sample_parameters.ObjectCount, 0, 0);
      command_buffer_w->endRenderPass();
#if ENABLE_IMGUI
      command_buffer_w->beginRenderPass(imgui_pass.begin_info(), vk::SubpassContents::eInline);
      m_imgui.render_frame(command_buffer_w, m_current_frame.m_resource_index COMMA_CWDEBUG_ONLY(debug_name_prefix("m_imgui")));
      command_buffer_w->endRenderPass();
#endif
      command_buffer_w->end();
      Dout(dc::vkframe, "End recording command buffer.");

      submit(command_buffer_w);
    } // Unlock command pool.

    Dout(dc::vkframe, "Leaving Window::DrawSample.");
  }

  //===========================================================================
  //
  // ImGui
  //
  //===========================================================================

  TestApplication const& application() const
  {
    return static_cast<TestApplication const&>(task::SynchronousWindow::application());
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
    ImGui::Begin(application().application_name().c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    static std::string const hardware_name = "Hardware: " + static_cast<std::string>(logical_device().vh_physical_device().getProperties().deviceName);
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

vulkan::ImageKind const Window::s_vector_image_kind{{ .format = vk::Format::eR16G16B16A16Sfloat }};
vulkan::ImageViewKind const Window::s_vector_image_view_kind{s_vector_image_kind, {}};

class WindowEvents : public vulkan::WindowEvents
{
 public:
  using vulkan::WindowEvents::WindowEvents;
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

  void prepare_physical_device_features(vk::PhysicalDeviceFeatures& features10, vk::PhysicalDeviceVulkan11Features& features11, vk::PhysicalDeviceVulkan12Features& features12) const override
  {
    features10.setDepthClamp(true);
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
    .combineQueueRequest({
//    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 8,                      // Only used when it can not be combined.
        .priority = 0.8,                                // Only used when it can not be combined.
        .windows = root_window_cookie1})                // This may only be used for window1.
#if 0
    // {2}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 2,
        .priority = 0.2,
        .windows = root_window_cookie2})
#endif
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

#if 0
    // Create a child window of root_window1. This has to be done before calling
    // `application.create_logical_device` below, which gobbles up the root_window1 pointer.
    root_window1->create_child_window<WindowEvents, SingleButtonWindow>(
        std::make_tuple([&](SingleButtonWindow& window)
          {
            Dout(dc::always, "TRIGGERED!");
            application.set_max_anisotropy(window.logical_device().max_sampler_anisotropy());
          }
        ),
#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
        {0, 0, 150, 150},
#else
        {0, 0, 150, 50},
#endif
        LogicalDevice::root_window_cookie1,
        "Button");
#endif

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
