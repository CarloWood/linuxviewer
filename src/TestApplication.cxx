#include "sys.h"
#include "TestApplication.h"
#include "SampleParameters.h"
#include "vulkan/FrameResourcesData.h"
#include "vulkan/VertexData.h"
#include "vulkan/VulkanWindow.h"
#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDeviceFeatures.h"
#include "vulkan/infos/DeviceCreateInfo.h"
#include "debug.h"
#include "debug/DebugSetName.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

using namespace linuxviewer;

class Window : public task::VulkanWindow
{
 public:
  using task::VulkanWindow::VulkanWindow;

  ~Window()
  {
    DoutEntering(dc::vulkan, "Window::~Window()");
    m_logical_device->wait_idle();
  }

 private:
  vk::UniqueRenderPass m_render_pass;
  vk::UniqueRenderPass m_post_render_pass;
  vk::UniquePipeline m_graphics_pipeline;
  vulkan::BufferParameters m_vertex_buffer;
  vulkan::BufferParameters m_instance_buffer;

  SampleParameters Parameters;

  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 10 frames per second.
    return threadpool::Interval<100, std::chrono::milliseconds>{};
  }

  void MouseMove(int x, int y) override
  {
    DoutEntering(dc::notice, "Window::MouseMove(" << x << ", " << y << ")");
  }

  void MouseClick(size_t button, bool pressed) override
  {
    DoutEntering(dc::notice, "Window::MouseClick(" << button << ", " << pressed << ")");
  }

  void ResetMouse() override
  {
    DoutEntering(dc::notice, "Window::ResetMouse()");
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
    DoutEntering(dc::notice, "Window::draw_frame() [" << this << "]");

    m_current_frame.m_resource_count = Parameters.FrameResourcesCount;  // Slider value.
    Dout(dc::notice, "m_current_frame.m_resource_count = " << m_current_frame.m_resource_count);
    auto frame_begin_time = std::chrono::high_resolution_clock::now();

    // Start frame - calculate times and prepare GUI.
    start_frame();

    // Acquire swapchain image and create a framebuffer.
    acquire_image(*m_render_pass);

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
    finish_frame(/* *m_current_frame.m_frame_resources->m_post_command_buffer,*/ *m_post_render_pass);

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);
    float float_frame_time = static_cast<float>(total_frame_time.count() * 0.001f);
    Parameters.TotalFrameTime = Parameters.TotalFrameTime * 0.99f + float_frame_time * 0.01f;

    Dout(dc::vulkan, "Leaving Window::draw_frame with total_frame_time = " << total_frame_time);
  }

  void DrawSample()
  {
    DoutEntering(dc::vulkan, "Window::DrawSample() [" << this << "]");
    auto frame_resources = m_current_frame.m_frame_resources;

    std::vector<vk::ClearValue> clear_values = {
      { .color = vk::ClearColorValue{ .float32 = {{ 0.0f, 0.0f, 0.0f, 1.0f }} } },
      { .depthStencil = vk::ClearDepthStencilValue{ .depth = 1.0f } }
    };

    auto swapchain_extent = swapchain().extent();

    vk::RenderPassBeginInfo render_pass_begin_info{
      .renderPass = *m_render_pass,
      .framebuffer = *frame_resources->m_framebuffer,
      .renderArea = {
        vk::Offset2D(),                                 // VkOffset2D                               offset
        swapchain_extent,                               // VkExtent2D                               extent
      },
      .clearValueCount = static_cast<uint32_t>(clear_values.size()),
      .pClearValues = clear_values.data()
    };

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

    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_pre_command_buffer(command_pool_w);

      Dout(dc::vulkan, "Start recording command buffer.");
      command_buffer_w->begin( { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit } );
      command_buffer_w->beginRenderPass( render_pass_begin_info, vk::SubpassContents::eInline );
      command_buffer_w->bindPipeline( vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline );
      command_buffer_w->setViewport( 0, { viewport } );
      command_buffer_w->setScissor( 0, { scissor } );
      command_buffer_w->bindVertexBuffers( 0, { *m_vertex_buffer.m_buffer, *m_instance_buffer.m_buffer }, { 0, 0 } );
      command_buffer_w->bindDescriptorSets( vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, { *m_descriptor_set.m_handle }, {} );
      command_buffer_w->pushConstants( *m_pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof( float ), &scaling_factor );
      command_buffer_w->draw( 6 * SampleParameters::s_quad_tessellation * SampleParameters::s_quad_tessellation, Parameters.ObjectsCount, 0, 0 );
      command_buffer_w->endRenderPass();
      command_buffer_w->end();
      Dout(dc::vulkan, "End recording command buffer.");

      vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &(*m_current_frame.m_frame_resources->m_image_available_semaphore),
        .pWaitDstStageMask = &wait_dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffer_w
      };

      Dout(dc::vulkan, "Submitting command buffer.");
      presentation_surface().vh_graphics_queue().submit( { submit_info }, vk::Fence() );
    } // Unlock command pool.

    Dout(dc::vulkan, "Leaving Window::DrawSample.");
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

    std::vector<vulkan::RenderPassSubpassData> subpass_descriptions = {
      {
        {},                                                         // std::vector<VkAttachmentReference> const& InputAttachments
        {                                                           // std::vector<VkAttachmentReference> const& ColorAttachments
          {
            .attachment = 0,
            .layout = vk::ImageLayout::eColorAttachmentOptimal
          }
        },
        {                                                           // VkAttachmentReference const& DepthStencilAttachment;
          .attachment = 1,
          .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
      }
    };

    std::vector<vk::SubpassDependency> dependencies = {
      {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion
      },
      {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        .dependencyFlags = vk::DependencyFlagBits::eByRegion
      }
    };

    // Render pass - from present_src to color_attachment.
    {
      std::vector<vulkan::RenderPassAttachmentData> attachment_descriptions = {
        {
          .m_format = swapchain().format(),
          .m_load_op = vk::AttachmentLoadOp::eClear,
          .m_store_op = vk::AttachmentStoreOp::eStore,
          .m_initial_layout = vk::ImageLayout::ePresentSrcKHR,
          .m_final_layout = vk::ImageLayout::ePresentSrcKHR //vk::ImageLayout::eColorAttachmentOptimal
        },
        {
          .m_format = task::VulkanWindow::s_default_depth_format,
          .m_load_op = vk::AttachmentLoadOp::eClear,
          .m_store_op = vk::AttachmentStoreOp::eStore,
          .m_initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
          .m_final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
      };
      m_render_pass = logical_device().create_render_pass(attachment_descriptions, subpass_descriptions, dependencies
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_render_pass")));
    }

    // Post-render pass - from color_attachment to present_src.
    {
      std::vector<vulkan::RenderPassAttachmentData> attachment_descriptions = {
        {
          .m_format = swapchain().format(),
          .m_load_op = vk::AttachmentLoadOp::eLoad,
          .m_store_op = vk::AttachmentStoreOp::eStore,
          .m_initial_layout = vk::ImageLayout::eColorAttachmentOptimal,
          .m_final_layout = vk::ImageLayout::ePresentSrcKHR
        },
        {
          .m_format = task::VulkanWindow::s_default_depth_format,
          .m_load_op = vk::AttachmentLoadOp::eDontCare,
          .m_store_op = vk::AttachmentStoreOp::eDontCare,
          .m_initial_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
          .m_final_layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
        }
      };
      m_post_render_pass = logical_device().create_render_pass(attachment_descriptions, subpass_descriptions, dependencies
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_post_render_pass")));
    }
  }

  void create_graphics_pipeline() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipeline() [" << this << "]");

    vk::UniqueShaderModule vertex_shader_module = logical_device().create_shader_module(m_application->resources_path() / "shaders/shader.vert.spv"
        COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::vertex_shader_module")));
    vk::UniqueShaderModule fragment_shader_module = logical_device().create_shader_module(m_application->resources_path() / "shaders/shader.frag.spv"
        COMMA_CWDEBUG_ONLY(debug_name_prefix("create_graphics_pipeline()::fragment_shader_module")));

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stage_create_infos = {
      // Vertex shader.
      {
        .flags = vk::PipelineShaderStageCreateFlags(0),
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = *vertex_shader_module,
        .pName = "main"
      },
      // Fragment shader.
      {
        .flags = vk::PipelineShaderStageCreateFlags(0),
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = *fragment_shader_module,
        .pName = "main"
      }
    };

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
        .offset = offsetof(vulkan::VertexData, Texcoords)
      },
      {
        .location = 2,
        .binding = vertex_binding_description[1].binding,
        .format = vk::Format::eR32G32B32A32Sfloat,
        .offset = 0
      }
    };

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
      .renderPass = *m_render_pass,
      .subpass = 0,
      .basePipelineHandle = vk::Pipeline{},
      .basePipelineIndex = -1
    };

    m_graphics_pipeline = logical_device().handle().createGraphicsPipelineUnique(vk::PipelineCache{}, pipeline_create_info).value;
    DebugSetName(m_graphics_pipeline, debug_name_prefix("m_graphics_pipeline"));
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
  }

  void prepare_logical_device(vulkan::DeviceCreateInfo& device_create_info) const override
  {
    using vulkan::QueueFlagBits;

    device_create_info
    // {0}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eGraphics,
//        .max_number_of_queues = 13,
        .priority = 1.0})
    // {1}
    .combineQueueRequest({
//    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 4,                      // Only used when it can not be combined.
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
    auto root_window1 = application.create_root_window<Window>({1000, 800}, LogicalDevice::root_window_cookie1);

    // Create a logical device that supports presenting to root_window1.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));

    // Assume logical_device also supports presenting on root_window2.
    auto root_window2 = application.create_root_window<Window>({400, 400}, LogicalDevice::root_window_cookie2, *logical_device, "Second window");

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
