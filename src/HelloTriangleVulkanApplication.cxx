#include "sys.h"
#include "HelloTriangleVulkanApplication.h"
#include "vulkan.old/InstanceCreateInfo.h"
#include "vulkan.old/DeviceCreateInfo.h"
#include "vulkan.old/CommandPoolCreateInfo.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

namespace utils { using namespace threading; }

void HelloTriangleVulkanApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "HelloTriangleVulkanApplication::on_menu_File_QUIT()");
  // Menu entry File --> Quit was called.

  // Close all windows and cause the application to return from run(),
  // which will terminate the application (see application.cxx).
  terminate();
}

void HelloTriangleVulkanApplication::on_main_instance_startup()
{
  DoutEntering(dc::notice, "HelloTriangleVulkanApplication::on_main_instance_startup()");
}

void HelloTriangleVulkanApplication::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  //FIXME - not implemented yet.
}

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  try
  {
    constexpr int number_of_threads = 4;
    constexpr int max_number_of_threads = 30;
    constexpr int reserved_threads = 1;

    ApplicationCreateInfo application_create_info;
    application_create_info
      // ApplicationCreateInfo
      .set_number_of_threads(number_of_threads, max_number_of_threads, reserved_threads)
      // vk::ApplicationInfo
      .setPApplicationName("HelloTriangleVulkanApplication")
      .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
      ;

    DebugUtilsMessengerCreateInfoEXT debug_create_info;
    debug_create_info
      .setMessageType(
          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
      ;

    // Create main application.
    HelloTriangleVulkanApplication application(application_create_info COMMA_CWDEBUG_ONLY(debug_create_info));

    // Parse the command line options.
    application.parse_command_line(argc, argv);

    gui::WindowCreateInfo main_window_create_info;
    main_window_create_info
      // gui::WindowCreateInfoExt
      .setTitle("Main window title")
      ;

    vulkan::PhysicalDeviceFeatures physical_device_features;
    physical_device_features
      // vk::PhysicalDeviceFeatures
      .setDepthClamp(VK_TRUE)
      ;

    vulkan::DeviceCreateInfo device_create_info(physical_device_features);
    using vulkan::QueueFlagBits;
    device_create_info
      // vulkan::DeviceCreateInfo
      .addQueueRequests({
          .queue_flags = QueueFlagBits::eGraphics,
          .max_number_of_queues = 14,
          .priority = 1.0})
      .addQueueRequests({
          .queue_flags = QueueFlagBits::ePresentation,
          .max_number_of_queues = 3,
          .priority = 0.3})
      .addQueueRequests({
          .queue_flags = QueueFlagBits::ePresentation,
          .max_number_of_queues = 2,
          .priority = 0.2})
#ifdef CWDEBUG
      .setDebugName("Vulkan Device")
#endif
      ;

    vulkan::QueueRequestIndex const graphics_queue_index(0);

    vulkan::CommandPoolCreateInfo command_pool_create_info;
    using vk::CommandPoolCreateFlagBits;
    command_pool_create_info
      // vulkan::CommandPoolCreateInfo
      .setQueueRequestIndex(graphics_queue_index)
#ifdef CWDEBUG
      .setDebugName("Command Pool")
#endif
      // vk::CommandPoolCreateInfo
      .setFlags(CommandPoolCreateFlagBits::eTransient | CommandPoolCreateFlagBits::eResetCommandBuffer)
      ;

    // Create all objects.
    application
      .create_main_window(std::move(main_window_create_info))
      CWDEBUG_ONLY(.create_debug_messenger(std::move(debug_create_info)))
      .create_vulkan_device(std::move(device_create_info))
      .create_swapchain()
      .create_pipeline()
      .create_command_buffers(std::move(command_pool_create_info))
      ;

    // Run GUI main loop.
    application.run();

    // Application terminated cleanly.
    application.join_event_loop();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, "\e[31m" << error << " caught in HelloTriangleVulkanApplication.cxx\e[0m");
  }

  Dout(dc::notice, "Leaving main()");
}
