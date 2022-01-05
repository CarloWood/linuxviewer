#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "vulkan/FrameResourcesData.h"
#include "vulkan/PhysicalDeviceFeatures.h"
#include "vulkan/infos/DeviceCreateInfo.h"
#include "vulkan/shaderbuilder/ShaderModule.h"
#include "vulkan/Pipeline.h"
#include "protocols/xmlrpc/response/LoginResponse.h"
#include "protocols/xmlrpc/request/LoginToSimulator.h"
#include "protocols/GridInfoDecoder.h"
#include "protocols/GridInfo.h"
#include "evio/protocol/xmlrpc/Encoder.h"
#include "evio/protocol/xmlrpc/Decoder.h"
#include "statefultask/AIEngine.h"
#include "xmlrpc-task/XML_RPC_MethodCall.h"
#include "evio/Socket.h"
#include "evio/File.h"
#include "evio/protocol/http.h"
#include "evio/protocol/EOFDecoder.h"
#include "utils/debug_ostream_operators.h"
#include "utils/threading/Gate.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <functional>
#include <regex>
#include <iterator>

namespace utils { using namespace threading; }
namespace http = evio::protocol::http;

class MySocket : public evio::Socket
{
 private:
  http::ResponseHeadersDecoder m_input_decoder;
  GridInfoDecoder m_grid_info_decoder;
  GridInfo m_grid_info;
  evio::protocol::xmlrpc::Decoder m_xml_rpc_decoder;
  xmlrpc::LoginResponse m_login_response;
  evio::OutputStream m_output_stream;

 public:
  MySocket() :
    m_input_decoder(
        {{"application/xml", m_grid_info_decoder},
         {"text/xml", m_xml_rpc_decoder}}
        ),
    m_grid_info_decoder(m_grid_info),
    m_xml_rpc_decoder(m_login_response)
  {
    set_source(m_output_stream);
    set_protocol_decoder(m_input_decoder);
  }

  evio::OutputStream& output_stream() { return m_output_stream; }
};

class MyTestFile : public evio::File
{
 private:
  LinuxViewerApplication* m_application;
  evio::protocol::xmlrpc::Decoder m_xml_rpc_decoder;
  xmlrpc::LoginResponse m_login_response;

 public:
  MyTestFile(LinuxViewerApplication* application) : m_application(application), m_xml_rpc_decoder(m_login_response)
  {
    set_protocol_decoder(m_xml_rpc_decoder);
  }

  void closed(int& CWDEBUG_ONLY(allow_deletion_count)) override
  {
    DoutEntering(dc::notice, "MyTestFile::closed({" << allow_deletion_count << "})");
    m_application->quit();
  }
};

class MyInputFile : public evio::File
{
 private:
  boost::intrusive_ptr<MySocket> m_linked_output_device;

 public:
  MyInputFile(boost::intrusive_ptr<MySocket> linked_output_device) : m_linked_output_device(std::move(linked_output_device)) { }

  // If the file is closed we wrote everything, so call flush on the socket.
  void closed(int& UNUSED_ARG(allow_deletion_count)) override { m_linked_output_device->flush_output_device(); }
};

class MyOutputFile : public evio::File
{
};

#if 0 // FIXME: not sure where this goes?
// Called when the main instance (as determined by the GUI) of the application is starting.
void LinuxViewerApplication::on_main_instance_startup()
{
  DoutEntering(dc::notice, "LinuxViewerApplication::on_main_instance_startup()");

  // Allow the main thread to wait until the test finished.
  utils::Gate test_finished;

#if 0
  // This performs a login to MG, using pre-formatted data from a file.
  xmlrpc::LoginToSimulatorCreate login_to_simulator;
  std::ifstream ifs("/home/carlo/projects/aicxx/linuxviewer/POST_login_boost.xml");
  boost::archive::text_iarchive ia(ifs);
  ia >> login_to_simulator;

  auto login_request = std::make_shared<xmlrpc::LoginToSimulator>(login_to_simulator);
  auto login_response = std::make_shared<xmlrpc::LoginResponse>();

  AIEndPoint end_point("misfitzgrid.com", 8002);

  boost::intrusive_ptr<task::XML_RPC_MethodCall> task = new task::XML_RPC_MethodCall(CWDEBUG_ONLY(true));
  task->set_end_point(std::move(end_point));
  task->set_request_object(login_request);
  task->set_response_object(login_response);
#endif

#if 0
  std::stringstream ss;
  evio::protocol::xmlrpc::Encoder encoder(ss);

  try
  {
    encoder << lts;
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  {
    std::regex hash_re("[0-9a-f]{32}");
    LibcwDoutScopeBegin(LIBCWD_DEBUGCHANNELS, libcwd::libcw_do, dc::notice)
    LibcwDoutStream << "Encoding: ";
    std::string const& text = ss.str();
    std::regex_replace(std::ostreambuf_iterator<char>(LibcwDoutStream), text.begin(), text.end(), hash_re, "********************************");
    LibcwDoutScopeEnd;
  }
#endif

#if 0
  // Run a test task.
  boost::intrusive_ptr<task::ConnectToEndPoint> task = new task::ConnectToEndPoint(CWDEBUG_ONLY(true));
  auto socket = evio::create<MySocket>();
  task->set_socket(socket);
  task->set_end_point(std::move(end_point));
  task->on_connected([socket](bool success){
      if (success)
      {
#if 0
        socket->output_stream() << "GET /get_grid_info HTTP/1.1\r\n"
                                   "Host: misfitzgrid.com:8002\r\n"
                                   "Accept-Encoding:\r\n"
                                   "Accept: application/xml\r\n"
                                   "Connection: close\r\n"
                                   "\r\n" << std::flush;
        socket->flush_output_device();
#else
        auto post_login_file = evio::create<MyInputFile>(socket);
        socket->set_source(post_login_file, 1000000, 500000, 1000000);
        post_login_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/POST_login.xml", std::ios_base::in);
#endif
      }
    });
#else
//  auto input_file = evio::create<MyTestFile>(this);
//  input_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/login_Response_formatted.xml", std::ios_base::in);
#endif

#if 0
  auto output_file = evio::create<MyOutputFile>();
  output_file->set_source(socket, 4096, 3000, std::numeric_limits<size_t>::max());
  output_file->open("/home/carlo/projects/aicxx/linuxviewer/linuxviewer/src/login_Response.xml", std::ios_base::out);
#endif

#if 0
  // This run the task that was created above.
  task->run([this, task, &test_finished](bool success){
      if (!success)
        Dout(dc::warning, "task::XML_RPC_MethodCall was aborted");
      else
        Dout(dc::notice, "Task with endpoint " << task->get_end_point() << " finished; response: " << *static_cast<xmlrpc::LoginResponse*>(task->get_response_object().get()));
      this->quit();
      test_finished.open();
    });

  test_finished.wait();
#endif
}
#endif

#if 0
void LinuxViewerApplication::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  using namespace menu_keys;

#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry}, std::bind(&LinuxViewerApplication::on_menu_##top##_##entry, this))

  //---------------------------------------------------------------------------
  // Menu buttons that have a call back to this object are added below.
  //

  //FIXME: not compiling with vulkan
  //ADD(File, QUIT);
}

void LinuxViewerApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "LinuxViewerApplication::on_menu_File_QUIT()");

  // Close all windows and cause the application to return from run(),
  // which will terminate the application (see application.cxx).
  terminate();
}
#endif

class Window : public task::SynchronousWindow
{
 public:
  using task::SynchronousWindow::SynchronousWindow;

 private:
  vk::UniquePipeline m_graphics_pipeline;
  vk::UniquePipelineLayout m_pipeline_layout;

  int m_frame_count = 0;

  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 10 frames per second.
    return threadpool::Interval<100, std::chrono::milliseconds>{};
  }

  size_t number_of_frame_resources() const override
  {
    return 3;
  }

  void draw_frame() override
  {
    DoutEntering(dc::vkframe, "Window::draw_frame() [frame:" << m_frame_count << "; " << this << "; " << (is_slow() ? "SlowWindow" : "Window") << "]");

    // Skip the first frame.
    if (++m_frame_count == 1)
      return;

    ASSERT(m_current_frame.m_resource_count == 3);
    Dout(dc::vkframe, "m_current_frame.m_resource_count = " << m_current_frame.m_resource_count);
    auto frame_begin_time = std::chrono::high_resolution_clock::now();

    // Start frame - calculate times and prepare GUI.
    start_frame();

    // Acquire swapchain image.
    acquire_image();                    // Can throw vulkan::OutOfDateKHR_Exception.

    // Draw scene/prepare scene's command buffers.
    {
      // Draw sample-specific data - includes command buffer submission!!
      DrawSample();
    }

    // Draw GUI and present swapchain image.
    finish_frame(vk::RenderPass{});

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);

    Dout(dc::vkframe, "Leaving Window::draw_frame with total_frame_time = " << total_frame_time);
  }

  void DrawSample()
  {
    DoutEntering(dc::vkframe, "Window::DrawSample() [" << this << "]");
    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;

    std::vector<vk::ClearValue> clear_values = {
      { .depthStencil = vk::ClearDepthStencilValue{ .depth = 1.0f } },
      { .color = vk::ClearColorValue{ .float32 = {{ 0.0f, 0.0f, 0.0f, 1.0f }} } }
    };

    auto swapchain_extent = swapchain().extent();

    std::array<vk::ImageView, 2> attachments = {
      *m_current_frame.m_frame_resources->m_depth_attachment.m_image_view,
      swapchain().vh_current_image_view()
    };
#ifdef CWDEBUG
    Dout(dc::vkframe, "m_swapchain.m_current_index = " << swapchain().current_index());
    std::array<vk::Image, 2> attachment_images = {
      swapchain().images()[swapchain().current_index()],
      *m_current_frame.m_frame_resources->m_depth_attachment.m_image
    };
    Dout(dc::vkframe, "Attachments: ");
    for (int i = 0; i < 2; ++i)
    {
      Dout(dc::vkframe, "  image_view: " << attachments[i] << ", image: " << attachment_images[i]);
    }
#endif
    vk::StructureChain<vk::RenderPassBeginInfo, vk::RenderPassAttachmentBeginInfo> render_pass_begin_info_chain(
      {
        .renderPass = swapchain().vh_render_pass(),
        .framebuffer = swapchain().vh_framebuffer(),
        .renderArea = {
          .offset = {},
          .extent = swapchain_extent
        },
        .clearValueCount = static_cast<uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data()
      },
      {
        .attachmentCount = attachments.size(),
        .pAttachments = attachments.data()
      }
    );
    vk::RenderPassBeginInfo& render_pass_begin_info = render_pass_begin_info_chain.get<vk::RenderPassBeginInfo>();

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

    m_logical_device->reset_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed });
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_pre_command_buffer(command_pool_w);

      Dout(dc::vkframe, "Start recording command buffer.");
      command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
      command_buffer_w->beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
      command_buffer_w->bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
      command_buffer_w->setViewport(0, { viewport });
      command_buffer_w->setScissor(0, { scissor });
      command_buffer_w->draw(3, 1, 0, 0);
      command_buffer_w->endRenderPass();
      command_buffer_w->end();
      Dout(dc::vkframe, "End recording command buffer.");

      vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = swapchain().vhp_current_image_available_semaphore(),
        .pWaitDstStageMask = &wait_dst_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = command_buffer_w,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = swapchain().vhp_current_rendering_finished_semaphore()
      };

      Dout(dc::vkframe, "Submitting command buffer: submit({" << submit_info << "}, " << *frame_resources->m_command_buffers_completed << ")");
      presentation_surface().vh_graphics_queue().submit( { submit_info }, *frame_resources->m_command_buffers_completed );
    } // Unlock command pool.

    Dout(dc::vkframe, "Leaving Window::DrawSample.");
  }

  void create_vertex_buffers() override
  {
    DoutEntering(dc::vulkan, "Window::create_vertex_buffers() [" << this << "]");
  }

  // Create renderpass / attachment objects.
  vulkan::rendergraph::Attachment const depth{attachment_id_context, s_depth_image_view_kind, "depth"};

  void create_render_passes() override
  {
    DoutEntering(dc::vulkan, "Window::create_render_passes() [" << this << "]");

    // These must be references.
    auto& final_pass = m_render_graph.create_render_pass("final_pass");
    auto& output = swapchain().presentation_attachment();

    m_render_graph = final_pass[~depth]->stores(~output);
    m_render_graph.generate(this);

    // Create the swapchain render pass.
    set_swapchain_render_pass(logical_device().create_render_pass(final_pass
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_swapchain.m_render_pass"))));
  }

  void create_descriptor_set() override
  {
    DoutEntering(dc::vulkan, "Window::create_descriptor_set() [" << this << "]");
  }

  void create_textures() override
  {
    DoutEntering(dc::vulkan, "Window::create_textures() [" << this << "]");
  }

  void create_pipeline_layout() override
  {
    DoutEntering(dc::vulkan, "Window::create_pipeline_layout() [" << this << "]");

    m_pipeline_layout = logical_device().create_pipeline_layout({}, {}
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline_layout")));
  }

  static constexpr std::string_view triangle_vert_glsl = R"glsl(
#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(-0.5, 0.5),
    vec2(0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main()
{
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  fragColor = colors[gl_VertexIndex];
}
)glsl";

  static constexpr std::string_view triangle_frag_glsl = R"glsl(
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;

void main()
{
  outColor = vec4(fragColor, 1.0);
}
)glsl";

  void create_graphics_pipeline() override
  {
    DoutEntering(dc::vulkan, "Window::create_graphics_pipeline() [" << this << "]");

    vulkan::Pipeline pipeline(this);

    {
      using namespace vulkan::shaderbuilder;

      ShaderCompiler compiler;
      ShaderCompilerOptions options;

      ShaderModule shader_vert(vk::ShaderStageFlagBits::eVertex);
      shader_vert.set_name("triangle.vert.glsl").load(triangle_vert_glsl).compile(compiler, options);
      pipeline.add(shader_vert);

      ShaderModule shader_frag(vk::ShaderStageFlagBits::eFragment);
      shader_frag.set_name("triangle.frag.glsl").load(triangle_frag_glsl).compile(compiler, options);
      pipeline.add(shader_frag);

//      pipeline.add(ShaderModule{vk::ShaderStageFlagBits::eVertex, "triangle.vert.glsl"}.load(triangle_vert_glsl).compile(compiler, options));
    }

    std::vector<vk::VertexInputBindingDescription> vertex_binding_description = {
    };

    std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions = {
    };

    vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info{
      .flags = {},
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_description.size()),
      .pVertexBindingDescriptions = vertex_binding_description.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size()),
      .pVertexAttributeDescriptions = vertex_attribute_descriptions.data()
    };

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{
      .flags = {},
      .topology = vk::PrimitiveTopology::eTriangleList,
      .primitiveRestartEnable = VK_FALSE
    };

    vk::Extent2D swapchain_extent = swapchain().extent();

    vk::Viewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = (float)swapchain_extent.width,
      .height = (float)swapchain_extent.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f
    };

    vk::Rect2D scissor{
      .offset = {0, 0},
      .extent = swapchain_extent
    };

    vk::PipelineViewportStateCreateInfo viewport_state_create_info{
      .flags = {},
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor
    };

    vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info{
      .flags = {},
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
      .flags = {},
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 1.0f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };

    vk::PipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
      .flags = {},
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
      .flags = {},
      .logicOpEnable = VK_FALSE,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &color_blend_attachment_state,
      .blendConstants = {{ 0.0f, 0.0f, 0.0f, 0.0f }}
    };

    std::vector<vk::DynamicState> dynamic_states = {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state_create_info{
      .flags = {},
      .dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
      .pDynamicStates = dynamic_states.data()
    };

    auto const& shader_stage_create_infos = pipeline.shader_stage_create_infos();

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
      .renderPass = swapchain().vh_render_pass(),
      .subpass = 0,
      .basePipelineHandle = vk::Pipeline{},
      .basePipelineIndex = -1
    };

    m_graphics_pipeline = logical_device().create_graphics_pipeline(vk::PipelineCache{}, pipeline_create_info
        COMMA_CWDEBUG_ONLY(debug_name_prefix("m_graphics_pipeline")));
  }
};

class WindowEvents : public vulkan::WindowEvents
{
 private:
  void MouseMove(int x, int y) override
  {
    DoutEntering(dc::notice, "WindowEvents::MouseMove(" << x << ", " << y << ")");
  }

  void MouseClick(size_t button, bool pressed) override
  {
    DoutEntering(dc::notice, "WindowEvents::MouseClick(" << button << ", " << pressed << ")");
  }

  void ResetMouse() override
  {
    DoutEntering(dc::notice, "WindowEvents::ResetMouse()");
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
        .queue_flags = QueueFlagBits::eGraphics})
    // {1}
    .combineQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation})
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
    LinuxViewerApplication application;

    // Initialize application using the virtual functions of TestApplication.
    application.initialize(argc, argv);

    // Create a window.
    auto root_window1 = application.create_root_window<WindowEvents, Window>({500, 800}, LogicalDevice::root_window_cookie1, "Main window title");

    // Create a logical device that supports presenting to root_window1.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));

    // Run the application.
    application.run();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, "\e[31m" << error << ", caught in LinuxViewerApplication.cxx\e[0m");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "\e[31mstd::exception: " << exception.what() << " caught in LinuxViewerApplication.cxx\e[0m");
  }
#endif

  Dout(dc::notice, "Leaving main()");
}
