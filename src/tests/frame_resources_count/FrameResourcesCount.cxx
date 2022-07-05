#include "sys.h"
#include "FrameResourcesCount.h"
#include "Window.h"
#include "LogicalDevice.h"
#include "../SingleButtonWindow.h"
#include "utils/debug_ostream_operators.h"
#include "debug.h"

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  try
  {
    // Create the application object.
    FrameResourcesCount application;

    // Initialize application using the virtual functions of FrameResourcesCount.
    application.initialize(argc, argv);

    // Create a window.
    auto root_window1 = application.create_root_window<vulkan::WindowEvents, Window>({1000, 800}, LogicalDevice::root_window_request_cookie1);

#if 0
    // Create a child window of root_window1. This has to be done before calling
    // `application.create_logical_device` below, which gobbles up the root_window1 pointer.
    root_window1->create_child_window<WindowEvents, SingleButtonWindow>(
        std::make_tuple([&](SingleButtonWindow& window)
          {
            Dout(dc::always, "TRIGGERED!");
            application.set_max_anisotropy(window.logical_device()->max_sampler_anisotropy());
          }
        ),
#if ADD_STATS_TO_SINGLE_BUTTON_WINDOW
        {0, 0, 150, 150},
#else
        {0, 0, 150, 50},
#endif
        LogicalDevice::root_window_request_cookie1,
        u8"Button");
#endif

    // Create a logical device that supports presenting to root_window1.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));

    // Assume logical_device also supports presenting on root_window2.
//    application.create_root_window<WindowEvents, SlowWindow>({400, 400}, LogicalDevice::root_window_request_cookie1, *logical_device, "Second window");

    // Run the application.
    application.run();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, "\e[31m" << error << ", caught in FrameResourcesCount.cxx\e[0m");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "\e[31mstd::exception: " << exception.what() << " caught in FrameResourcesCount.cxx\e[0m");
  }
#endif

  Dout(dc::notice, "Leaving main()");
}
