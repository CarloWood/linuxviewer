#include "sys.h"
#include "HelloTriangle.h"
#include "LogicalDevice.h"
#include "Window.h"
#include "debug.h"
#ifdef CWDEBUG
#include <utils/debug_ostream_operators.h>      // Required to print AIAlert::Error to an ostream.
#endif

// Required header to be included below all other headers (that might include vulkan headers).
#include <vulkan/lv_inline_definitions.h>

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  try
  {
    // Create the application object.
    HelloTriangle application;

    // Initialize application using the virtual functions of FrameResourcesCount.
    application.initialize(argc, argv);

    // Create a window.
    auto root_window = application.create_root_window<vulkan::WindowEvents, Window>({400, 400}, LogicalDevice::root_window_request_cookie);

    // Create a logical device that supports presenting to root_window.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window));

    // Run the application.
    application.run();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, error << ", caught in HelloTriangle.cxx");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "std::exception: " << exception.what() << " caught in HelloTriangle.cxx");
  }
#endif

  Dout(dc::notice, "Leaving main()");
}
