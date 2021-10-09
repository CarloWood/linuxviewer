#pragma once

#include "QueueReply.h"
#include "PresentationSurface.h"
#include "Swapchain.h"
#include "statefultask/Broker.h"
#include "statefultask/TaskEvent.h"
#include "xcb-task/XcbConnection.h"
#include "threadpool/Timer.h"
#include <vulkan/vulkan.hpp>
#include <memory>

namespace xcb {
class ConnectionBrokerKey;
} // namespace xcb

namespace vulkan {
class Application;
} // namespace vulkan

namespace linuxviewer::OS {
class Window;
} // namespace linuxviewer::OS

namespace task {

class LogicalDevice;

/**
 * The VulkanWindow task.
 *
 */
class VulkanWindow : public AIStatefulTask
{
 public:
  using xcb_connection_broker_type = task::Broker<task::XcbConnection, std::function<void()>>;
  using window_cookie_type = vulkan::QueueReply::window_cookies_type;

  // This event is triggered as soon as m_window is created.
  statefultask::TaskEvent m_window_created_event;

 private:
  static constexpr condition_type connection_set_up = 1;
  static constexpr condition_type frame_timer = 2;
  static constexpr condition_type logical_device_index_available = 4;

  // Constructor
  vulkan::Application* m_application;
  std::unique_ptr<linuxviewer::OS::Window> m_window;                            // Initialized in VulkanWindow_create.

  // set_title
  std::string m_title;
  // set_size
  vk::Extent2D m_extent;
  // set_logical_device
  LogicalDevice const* m_logical_device = nullptr;
  // set_xcb_connection
  boost::intrusive_ptr<xcb_connection_broker_type> m_broker;
  xcb::ConnectionBrokerKey const* m_broker_key;

  // run
  window_cookie_type m_window_cookie = {};                                      // Unique bit for the creation event of this window (as determined by the user).
  boost::intrusive_ptr<task::XcbConnection const> m_xcb_connection_task;        // Initialized in VulkanWindow_start.
  std::atomic_bool m_must_close = false;
  std::atomic_int m_logical_device_index = -1;                                  // Index into Application::m_logical_device_list.
                                                                                // Initialized in LogicalDevice_create by call to Application::create_device.
  vulkan::PresentationSurface m_presentation_surface;                           // The presentation surface information (surface-, graphics- and presentation queue handles).
  vulkan::Swapchain m_swapchain;                                                // The swap chain used for this surface.

  threadpool::Timer::Interval m_frame_rate_interval;                            // The minimum time between two frames.
  threadpool::Timer m_frame_rate_limiter;

#ifdef CWDEBUG
  bool const mVWDebug;                                                          // A copy of mSMDebug.
#endif

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum vulkan_window_state_type {
    VulkanWindow_xcb_connection = direct_base_type::state_end,
    VulkanWindow_create,
    VulkanWindow_logical_device_index_available,
    VulkanWindow_create_swapchain,
    VulkanWindow_render_loop,
    VulkanWindow_close
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = VulkanWindow_close + 1;

  /// Construct an VulkanWindow object.
  VulkanWindow(vulkan::Application* application, std::unique_ptr<linuxviewer::OS::Window>&& window COMMA_CWDEBUG_ONLY(bool debug = false));

  void set_title(std::string&& title)
  {
    m_title = std::move(title);
  }

  void set_size(vk::Extent2D extent)
  {
    m_extent = extent;
  }

  void set_logical_device(LogicalDevice const* logical_device)
  {
    m_logical_device = logical_device;
  }

  void set_frame_rate_interval(threadpool::Timer::Interval frame_rate_interval)
  {
    m_frame_rate_interval = frame_rate_interval;
  }

  window_cookie_type window_cookie() const
  {
    return m_window_cookie;
  }

  void set_window_cookie(window_cookie_type window_cookie)
  {
    m_window_cookie = window_cookie;
  }

  // The broker_key object must have a life-time longer than the time it takes to finish task::XcbConnection.
  void set_xcb_connection(boost::intrusive_ptr<xcb_connection_broker_type> broker, xcb::ConnectionBrokerKey const* broker_key)
  {
    m_broker = std::move(broker);
    m_broker_key = broker_key;
  }

  // Called by Application::create_device.
  void set_logical_device_index(int index)
  {
    // Do not pass a root_window to Application::create_logical_device that was already associated with a logical device
    // by passing a logical device to Application::create_root_window that created it.
    //
    // You either call:
    //
    // auto root_window1 = application.create_root_window(std::make_unique<Window>(), {400, 400}, LogicalDevice::window_cookie1);
    // auto logical_device = application.create_logical_device(std::make_unique<LogicalDevice>(), root_window1);
    //
    // OR
    //
    // auto root_window2 = application.create_root_window(std::make_unique<Window>(), {400, 400}, LogicalDevice::window_cookie2, *logical_device);
    // // Do not pass root_window2 to create_logical_device.
    //
    ASSERT(m_logical_device_index.load(std::memory_order::relaxed) == -1);
    m_logical_device_index.store(index, std::memory_order::relaxed);
    signal(logical_device_index_available);
  }

  void close()
  {
    // Tell the task to finish the next time it enters the render loop.
    m_must_close = true;
  }

  // Accessors.
  vk::SurfaceKHR vh_surface() const;

  int get_logical_device_index() const
  {
    // Bug in linuxviewer. This should never happen: set_logical_device_index()
    // should always be called before this accessor is used.
    int logical_device_index = m_logical_device_index.load(std::memory_order::relaxed);
    ASSERT(logical_device_index != -1);
    return logical_device_index;
  }

 private:
  void create_swapchain();

 protected:
  /// Call finish() (or abort()), not delete.
  ~VulkanWindow() override;

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override final;

  /// Set up task for running.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;

  /// Close window.
  void finish_impl() override;
};

} // namespace task
