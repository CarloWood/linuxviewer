#include "sys.h"
#include "TestApplication.h"
#include "vulkan/OperatingSystem.h"
#include "debug.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

using namespace linuxviewer;

class Window : public OS::Window
{
  threadpool::Timer::Interval get_frame_rate_interval() const override
  {
    // Limit the frame rate of this window to 10 frames per second.
    return threadpool::Interval<100, std::chrono::milliseconds>{};
  }

  void OnWindowSizeChanged() override
  {
    DoutEntering(dc::notice, "Window::OnWindowSizeChanged()");
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

#if 0
  void Draw() override
  {
    DoutEntering(dc::notice, "Window::Draw()");
  }

  bool ReadyToDraw() const override
  {
    DoutEntering(dc::notice, "Window::ReadyToDraw()");
    return false;
  }
#endif
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
    application.create_main_window(std::make_unique<Window>(), {1000, 800});
    application.create_main_window(std::make_unique<Window>(), {400, 400}, "Second window");

    // Run the application.
    application.run(argc, argv);
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
