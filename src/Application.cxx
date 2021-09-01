#include "sys.h"
#include "Application.h"
#include "Window.h"
#include "vulkan.old/check_instance_extensions_availability.h"
#include <vector>

Application::Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo& instance_create_info
      COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT& debug_create_info)) :
  gui::Application(application_create_info.pApplicationName),
  m_mpp(application_create_info.block_size, application_create_info.minimum_chunk_size, application_create_info.maximum_chunk_size),
  m_thread_pool(application_create_info.number_of_threads, application_create_info.max_number_of_threads),
  m_high_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
  m_medium_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
  m_low_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity)),
  m_event_loop(m_low_priority_queue COMMA_CWDEBUG_ONLY(application_create_info.event_loop_color, application_create_info.color_off_code)),
  m_resolver_scope(m_low_priority_queue, false),
  m_return_from_run(false),
  m_gui_idle_engine("gui_idle_engine", application_create_info.max_duration)
{
  // Call this before print the DoutEntering debug output, so we get to see all extensions that are used.
  instance_create_info.add_extensions(glfw::getRequiredInstanceExtensions());

  DoutEntering(dc::notice, "Application::Application(" << application_create_info << ", " << instance_create_info.base() << ", " << debug_create_info  << ")");

#ifdef CWDEBUG
  // Set the debug color function (this couldn't be part of the above constructor).
  m_thread_pool.set_color_functions(application_create_info.thread_pool_color_function);
  // Turn on required debug channels.
  Application::debug_init();
  // Set debug call back function to Application::debugCallback.
  debug_create_info.setup_callback(this);
  // Added debug_create_info as extension to use during Instance creation and destruction.
  VkDebugUtilsMessengerCreateInfoEXT& ref = debug_create_info;
  instance_create_info.setPNext(&ref);
#endif

  // Upon return from this constructor, the application_create_info might be destucted before we get another chance
  // to create the vulkan instance (this is unlikely, but certainly not impossible). And since instance_create_info contains
  // a pointer to it, as well as might itself be destructed, we need to be done with both of them before returning.
  // Hence, the instance has to be created here.
  createInstance(instance_create_info);
}

void Application::createInstance(vulkan::InstanceCreateInfo const& instance_create_info)
{
  if (vulkan::InstanceCreateInfo::s_enableValidationLayers && !vulkan::InstanceCreateInfo::checkValidationLayerSupport())
    throw std::runtime_error("validation layers requested, but not available!");

  // Check that all required extensions are available.
  vulkan::check_instance_extensions_availability(instance_create_info.enabled_extension_names());

  Dout(dc::vulkan|continued_cf|flush_cf, "Calling vk::createInstanceUnique()... ");
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif
  m_instance = vk::createInstanceUnique(instance_create_info.base());
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
#endif
  Dout(dc::finish, "done (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms)");
  // Mandatory call after creating the vulkan instance.
  m_dispatch_loader.load(*m_instance);
}

void Application::initialization_state(InitializationState const state)
{
  // You are not allowed to call the initialization functions
  // (parse_command_line, create_main_window, create_debug_messenger, etc -- see header)
  // in the wrong order.
  ASSERT(m_ist_state < state);

  // Provide defaults.
  for (int next = m_ist_state + 1; next != state; ++next)
  {
    InitializationState ist = static_cast<InitializationState>(next);
    switch (ist)
    {
      case istParseCommandLine:
        Dout(dc::notice, "Application::parse_command_line was not called.");
        m_ist_state = istParseCommandLine;
        break;
      case istMainWindow:
        create_main_window(gui::WindowCreateInfo{});
        break;
#ifdef CWDEBUG
      case istDebugMessenger:
        create_debug_messenger(DebugUtilsMessengerCreateInfoEXT{});
        break;
#endif
      case istVulkanDevice:
        create_vulkan_device(vulkan::DeviceCreateInfo{});
        break;
      case istSwapchain:
        create_swapchain();
        break;
      case istPipeline:
        create_pipeline();
        break;
      case istCommandBuffers:
        create_command_buffers(vulkan::CommandPoolCreateInfo{});
        break;
      default:
        break;
    }
  }

  m_ist_state = state;
}

Application& Application::parse_command_line(int argc, char* argv[])
{
  initialization_state(istParseCommandLine);

#ifdef CWDEBUG
  DoutEntering(dc::notice|flush_cf|continued_cf, "Application::init(" << argc << ", ");
  // Print the commandline arguments used.
  char const* prefix = "{";
  for (char** arg = argv; *arg; ++arg)
  {
    Dout(dc::continued|flush_cf, prefix << '"' << *arg << '"');
    prefix = ", ";
  }
  Dout(dc::finish|flush_cf, "})");
#endif
  return *this;
}

Application& Application::create_main_window(gui::WindowCreateInfo&& main_window_create_info)
{
  initialization_state(istMainWindow);

  // If we get here then this application is the main process and owns the (a) main window.
  // Call the base class methos create_main_window to create the main window.
  createMainWindow<Window>(main_window_create_info);
  return *this;
}

#ifdef CWDEBUG
Application& Application::create_debug_messenger(DebugUtilsMessengerCreateInfoEXT&& debug_create_info)
{
  initialization_state(istDebugMessenger);

  // In case this was a second temporary (the one passed to Application then was the first),
  // initialize the callback pointers again.
  debug_create_info.setup_callback(this);

  // Set up the debug messenger so that debug output generated by the validation layers
  // is redirected to debugCallback().
  m_debug_messenger.prepare(*m_instance, debug_create_info);
  return *this;
}
#endif

Application& Application::create_vulkan_device(vulkan::DeviceCreateInfo&& device_create_info)
{
  initialization_state(istVulkanDevice);

  // The m_vulkan_device draws to m_main_window.
  m_vulkan_device.prepare(*m_instance, m_dispatch_loader, main_window_ptr()->vh_surface(), std::move(device_create_info));

  // Call virtual function that allows the derived application to extract the queue handles it needs.
  init_queue_handles();
  return *this;
}

Application& Application::create_swapchain()
{
  initialization_state(istSwapchain);

  create_swapchain_impl();
  return *this;
}

Application& Application::create_pipeline()
{
  initialization_state(istPipeline);

  vk::PipelineLayout vh_pipeline_layout = createPipelineLayout(m_vulkan_device);
  createPipeline(m_vulkan_device, main_window_ptr()->swapchain_ptr(), vh_pipeline_layout);
  m_vulkan_device->destroyPipelineLayout(vh_pipeline_layout);
  return *this;
}

Application& Application::create_command_buffers(vulkan::CommandPoolCreateInfo&& command_pool_create_info)
{
  initialization_state(istCommandBuffers);

  m_vulkan_device.create_command_pool(command_pool_create_info);
  main_window()->createCommandBuffers(m_vulkan_device, m_pipeline.get());

  return *this;
}

void Application::run()
{
#ifdef CWDEBUG
  Dout(dc::notice, "Entering Application::run()");
  debug::Mark mark_running(u8"⥁");
#endif

  // Call any missing initialization functions.
  initialization_state(istRun);

  // Run the GUI main loop.
  while (running())
  {
    glfw::pollEvents();
    main_window()->draw_frame();
  }

  // Block until all GPU operations have completed.
  m_vulkan_device->waitIdle();
}

std::shared_ptr<Window> Application::main_window() const
{
  return std::static_pointer_cast<Window>(m_main_window);
}

vk::PipelineLayout Application::createPipelineLayout(vulkan::Device const& device)
{
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  vk::PipelineLayout vh_pipelineLayout = device->createPipelineLayout(pipelineLayoutInfo);
  return vh_pipelineLayout;
}

void Application::createPipeline(vulkan::Device const& device, vulkan::HelloTriangleSwapchain const* swapchain_ptr, vk::PipelineLayout vh_pipeline_layout)
{
  vulkan::PipelineCreateInfo pipelineConfig{};
  vulkan::Pipeline::defaultPipelineCreateInfo(pipelineConfig, swapchain_ptr->width(), swapchain_ptr->height());
  pipelineConfig.m_vh_render_pass = swapchain_ptr->vh_render_pass();
  pipelineConfig.m_vh_pipeline_layout = vh_pipeline_layout;
  m_pipeline = std::make_unique<vulkan::Pipeline>(device, SHADERS_DIR "/simple_shader.vert.spv", SHADERS_DIR "/simple_shader.frag.spv", pipelineConfig);
}

void Application::quit()
{
  DoutEntering(dc::notice, "Application::quit()");
  m_return_from_run = true;
}

bool Application::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}

#ifdef CWDEBUG

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vkverbose("VKVERBOSE");
channel_ct vkinfo("VKINFO");
channel_ct vkwarning("VKWARNING");
channel_ct vkerror("VKERROR");
NAMESPACE_DEBUG_CHANNELS_END
#endif

void Application::debug_init()
{
  if (!DEBUGCHANNELS::dc::vkerror.is_on())
    DEBUGCHANNELS::dc::vkerror.on();
  if (!DEBUGCHANNELS::dc::vkwarning.is_on() && DEBUGCHANNELS::dc::warning.is_on())
    DEBUGCHANNELS::dc::vkwarning.on();
}

// Default callback function for debug output from vulkan layers.
void Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData)
{
  char const* color_end = "";
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    Dout(dc::vkerror|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
    color_end = "\e[0m";
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    Dout(dc::vkwarning|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
    color_end = "\e[0m";
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    Dout(dc::vkinfo|continued_cf, pCallbackData->pMessage);
  else
    Dout(dc::vkverbose|continued_cf, pCallbackData->pMessage);

  if (pCallbackData->objectCount > 0)
  {
    Dout(dc::continued, " [with an objectCount of " << pCallbackData->objectCount << "]");
    for (int i = 0; i < pCallbackData->objectCount; ++i)
    {
      Dout(dc::vulkan, static_cast<vk::DebugUtilsObjectNameInfoEXT>(pCallbackData->pObjects[i]));
    }
  }

  Dout(dc::finish, color_end);
}
#endif
