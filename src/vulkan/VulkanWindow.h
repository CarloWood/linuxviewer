#pragma once

#include "statefultask/Broker.h"
#include "xcb-task/XcbConnection.h"
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

/**
 * The VulkanWindow task.
 *
 */
class VulkanWindow : public AIStatefulTask
{
 private:
  static constexpr condition_type connection_set_up = 1;

  // Constructor
  vulkan::Application* m_application;
  std::unique_ptr<linuxviewer::OS::Window> m_window;                            // Initialized in VulkanWindow_create

  // set_title
  std::string m_title;
  // set_size
  vk::Extent2D m_extent;
  // set_xcb_connection
  boost::intrusive_ptr<task::Broker<task::XcbConnection>> m_broker;
  xcb::ConnectionBrokerKey const* m_broker_key;

  // run
  boost::intrusive_ptr<task::XcbConnection const> m_xcb_connection_task;        // VulkanWindow_start
  std::atomic_bool m_must_close = false;

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum vulkan_window_state_type {
    VulkanWindow_xcb_connection = direct_base_type::state_end,
    VulkanWindow_create,
    VulkanWindow_render_loop,
    VulkanWindow_close
  };

 public:
  /// One beyond the largest state of this task.
  static state_type constexpr state_end = VulkanWindow_close + 1;

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

  // The broker_key object must have a life-time longer than the time it takes to finish task::XcbConnection.
  void set_xcb_connection(boost::intrusive_ptr<task::Broker<task::XcbConnection>> broker, xcb::ConnectionBrokerKey const* broker_key)
  {
    m_broker = broker;
    m_broker_key = broker_key;
  }

  void close()
  {
    // Tell the task to finish the next time it enters the render loop.
    m_must_close = true;
  }

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
