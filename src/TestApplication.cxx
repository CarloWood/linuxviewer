#include "sys.h"
#include "TestApplication.h"
#include "vulkan/OperatingSystem.h"
#include "debug.h"

using namespace linuxviewer;

class Window : public OS::Window
{
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

  // Create the application object.
  TestApplication application;

  // Initialize application using the virtual functions of TestApplication.
  application.initialize(argc, argv);

  // Create a window.
  application.create_main_window(std::make_unique<Window>(), "TestApplication", {1000, 800});

  // Run the application.
  application.run(argc, argv);
}
