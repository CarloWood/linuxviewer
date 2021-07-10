#include "sys.h"
#include "HelloTriangleVulkanApplication.h"
#include "utils/debug_ostream_operators.h"

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

bool HelloTriangleVulkanApplication::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  ApplicationCreateInfo create_info = {
  };

  // Create main application.
  HelloTriangleVulkanApplication application(create_info);
  try
  {
    // Run main application.
    application.run(argc, argv);

    // Application terminated cleanly.
    application.join_event_loop();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " caught in HelloTriangleVulkanApplication.cxx");
  }

  Dout(dc::notice, "Leaving main()");
}
