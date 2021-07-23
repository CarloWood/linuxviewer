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

  ApplicationCreateInfo application_create_info;
  application_create_info
    // ApplicationCreateInfo
    .set_number_of_threads(16, 32, 2)
    // vk::ApplicationInfo
    .setPApplicationName("HelloTriangleVulkanApplication")
    .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
    ;

  vulkan::InstanceCreateInfo const instance_create_info(application_create_info);

  // Create main application.
  HelloTriangleVulkanApplication application(application_create_info, instance_create_info);

  WindowCreateInfo main_window_create_info = {
    // gui::WindowCreateInfo
    {
      // glfw::WindowHints
      { .resizable = false,
        .focused = false,
        .centerCursor = false,
        .clientApi = glfw::ClientApi::None },
      // gui::WindowCreateInfoExt
      { .width = 500,
        .height = 800,
        .title = "Main window title" }
    }
  };

  try
  {
    // Run main application.
    application.run(argc, argv, main_window_create_info);

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
