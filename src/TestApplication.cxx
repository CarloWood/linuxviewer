#include "sys.h"
#include "TestApplication.h"
#include "vulkan/OperatingSystem.h"
#include "vulkan/LogicalDevice.h"
#include "vulkan/PhysicalDeviceFeatures.h"
#include "vulkan/infos/DeviceCreateInfo.h"
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

  void OnWindowSizeChanged(uint32_t width, uint32_t height) override
  {
    DoutEntering(dc::notice, "Window::OnWindowSizeChanged(" << width << ", " << height << ")");
    // MUST call the base class implementation.
    OS::Window::OnWindowSizeChanged(width, height);
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

class LogicalDevice : public vulkan::LogicalDevice
{
 public:
  // Every time create_root_window is called a cookie must be passed.
  // This cookie will be passed back to the virtual function ... when
  // querying what presentation queue family to use for that window (and
  // related windows).
  static constexpr int root_window_cookie1 = 1;
  static constexpr int root_window_cookie2 = 2;

  LogicalDevice()
  {
    DoutEntering(dc::notice, "LogicalDevice::LogicalDevice() [" << this << "]");
  }

  ~LogicalDevice() override
  {
    DoutEntering(dc::notice, "LogicalDevice::~LogicalDevice() [" << this << "]");
  }

  void prepare_physical_device_features(vulkan::PhysicalDeviceFeatures& physical_device_features) const override
  {
    // Use the setters from vk::PhysicalDeviceFeatures.
    physical_device_features.setDepthClamp(true);
  }

  void prepare_logical_device(vulkan::DeviceCreateInfo& device_create_info) const override
  {
    using vulkan::QueueFlagBits;

    device_create_info
    // {0}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eGraphics,
//        .max_number_of_queues = 13,
        .priority = 1.0})
    // {1}
    .combineQueueRequest({
//    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 4,                      // Only used when it can not be combined.
        .priority = 0.8,                                // Only used when it can not be combined.
        .windows = root_window_cookie1})                // This may only be used for window1.
    // {2}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 2,
        .priority = 0.2,
        .windows = root_window_cookie2})
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
    TestApplication application;

    // Initialize application using the virtual functions of TestApplication.
    application.initialize(argc, argv);

    // Create a window.
    auto root_window1 = application.create_root_window(std::make_unique<Window>(), {1000, 800}, LogicalDevice::root_window_cookie1);

    // Create a logical device that supports presenting to root_window1.
    auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), std::move(root_window1));

    // Assume logical_device also supports presenting on root_window2.
    auto root_window2 = application.create_root_window(std::make_unique<Window>(), {400, 400}, LogicalDevice::root_window_cookie2, *logical_device, "Second window");

    // Run the application.
    application.run();
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
