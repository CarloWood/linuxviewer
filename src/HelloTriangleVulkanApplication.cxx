#include "sys.h"
#include "HelloTriangleVulkanApplication.h"
#include "vulkan/InstanceCreateInfo.h"
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

  // Create main application.
  HelloTriangleVulkanApplication application(application_create_info);

  WindowCreateInfo main_window_create_info(
      // glfw::WindowHints
      { .resizable = false,
        .focused = false,
        .centerCursor = false,
        .clientApi = glfw::ClientApi::None });
  main_window_create_info
    // gui::WindowCreateInfoExt
    .setTitle("Main window title")
    ;

#ifdef CWDEBUG
  vulkan::DebugUtilsMessengerCreateInfoEXT debug_create_info;
  debug_create_info
    .setMessageSeverity(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    .setMessageType(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
        vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
        vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
    ;
#endif

  try
  {
    // Run main application.
    application.run(argc, argv, main_window_create_info COMMA_CWDEBUG_ONLY(debug_create_info));

    // Application terminated cleanly.
    application.join_event_loop();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, error << " caught in HelloTriangleVulkanApplication.cxx");
  }

  Dout(dc::notice, "Leaving main()");
}
