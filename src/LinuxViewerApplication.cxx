#include "sys.h"
#include "LinuxViewerApplication.h"
#include "LinuxViewerMenuBar.h"
#include "vulkan/FrameResourcesData.h"
#include "vulkan/infos/DeviceCreateInfo.h"
#include "vulkan/pipeline/Pipeline.h"
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
  // Create renderpass / attachment objects.
  RenderPass final_pass{this, "final_pass"};
  Attachment depth{this, "depth", s_depth_image_view_kind};

  // Define pipeline objects.
  vulkan::pipeline::Pipeline m_pipeline;

  vk::UniquePipeline m_graphics_pipeline;
  vk::UniquePipelineLayout m_pipeline_layout;

  int m_frame_count = 0;

 private:
  void create_render_graph() override
  {
    DoutEntering(dc::vulkan, "Window::create_render_graph() [" << this << "]");

    // These must be references.
    auto& output = swapchain().presentation_attachment();

    m_render_graph = final_pass[~depth]->stores(~output);
    m_render_graph.generate(this);
  }

  vulkan::FrameResourceIndex number_of_frame_resources() const override
  {
    return vulkan::FrameResourceIndex{3};
  }

  void create_descriptor_set() override { }
  void create_textures() override { }

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

    // The pipeline needs to know who owns it.
    m_pipeline.owning_window(this);

    {
      using namespace vulkan::shaderbuilder;

      ShaderInfo shader_vert(vk::ShaderStageFlagBits::eVertex, "triangle.vert.glsl");
      ShaderInfo shader_frag(vk::ShaderStageFlagBits::eFragment, "triangle.frag.glsl");

      shader_vert.load(triangle_vert_glsl);
      shader_frag.load(triangle_frag_glsl);

      ShaderCompiler compiler;

      m_pipeline.build_shader(shader_vert, compiler
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline")));
      m_pipeline.build_shader(shader_frag, compiler
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_pipeline")));
    }

    auto vertex_binding_description = m_pipeline.vertex_binding_descriptions();
    auto vertex_attribute_descriptions = m_pipeline.vertex_attribute_descriptions();

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

    auto const& shader_stage_create_infos = m_pipeline.shader_stage_create_infos();

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

  //===========================================================================
  //
  // Called from initialize_impl.
  //
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 10 frames per second.
    return threadpool::Interval<100, std::chrono::milliseconds>{};
  }

  //===========================================================================
  //
  // Frame code (called every frame)
  //
  //===========================================================================

  void draw_frame() override
  {
    DoutEntering(dc::vkframe, "Window::draw_frame() [frame:" << m_frame_count << "; " << this << "]");

    // Skip the first frame.
    if (++m_frame_count == 1)
      return;

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
    finish_frame();

    auto total_frame_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - frame_begin_time);

    Dout(dc::vkframe, "Leaving Window::draw_frame with total_frame_time = " << total_frame_time);
  }

  void DrawSample()
  {
    DoutEntering(dc::vkframe, "Window::DrawSample() [" << this << "]");
    vulkan::FrameResourcesData* frame_resources = m_current_frame.m_frame_resources;

    auto swapchain_extent = swapchain().extent();
    final_pass.update_image_views(swapchain(), frame_resources);

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

//    float scaling_factor = static_cast<float>(swapchain_extent.width) / static_cast<float>(swapchain_extent.height);

    m_logical_device->reset_fences({ *m_current_frame.m_frame_resources->m_command_buffers_completed });
    {
      // Lock command pool.
      vulkan::FrameResourcesData::command_pool_type::wat command_pool_w(frame_resources->m_command_pool);

      // Get access to the command buffer.
      auto command_buffer_w = frame_resources->m_command_buffer(command_pool_w);

      Dout(dc::vkframe, "Start recording command buffer.");
      command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
      command_buffer_w->beginRenderPass(final_pass.begin_info(), vk::SubpassContents::eInline);
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
};

class WindowEvents : public vulkan::WindowEvents
{
 public:
  using vulkan::WindowEvents::WindowEvents;
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
