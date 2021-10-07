#pragma once

#include "DispatchLoader.h"
#include "VulkanWindow.h"
#include "QueueReply.h"
#include <boost/intrusive_ptr.hpp>

namespace vulkan {

class PhysicalDeviceFeatures;
class DeviceCreateInfo;

// The collection of queue family properties for a given physical device.
class QueueFamilies
{
 private:
  utils::Vector<QueueFamilyProperties> m_queue_families;

 public:
  // Check that for every required feature in device_create_info, there is at least one queue family that supports it.
  bool is_compatible_with(DeviceCreateInfo const& device_create_info, utils::Vector<QueueReply, QueueRequestIndex>& queue_replies);

  QueueFamilyProperties const& operator[](QueueFamilyPropertiesIndex index) const
  {
    return m_queue_families[index];
  }

  // Construct a vector of QueueFamilyProperties for physical_device and surface (for the presentation capability bit).
  QueueFamilies(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface);

  // Construct an empty vector.
  QueueFamilies() = default;
};

struct UnsupportedQueueFlagsException : std::exception {
};

class LogicalDevice
{
 private:
  vk::PhysicalDevice m_vh_physical_device;      // The underlaying physical device.
  vk::UniqueDevice m_device;                    // A handle to the logical device.
  utils::Vector<QueueReply, QueueRequestIndex> m_queue_replies;
  QueueFamilies m_queue_families;
#ifdef CWDEBUG
  std::string m_debug_name;
#endif

 public:
  virtual ~LogicalDevice() = default;

  void prepare(vk::Instance vulkan_instance, DispatchLoader& dispatch_loader, task::VulkanWindow const* window_task_ptr);

  void print_on(std::ostream& os) const { char const* prefix = ""; os << '{'; print_members(os, prefix); os << '}'; }
  void print_members(std::ostream& os, char const* prefix) const;

#ifdef CWDEBUG
  void set_debug_name(std::string debug_name) { m_debug_name = std::move(debug_name); }
  std::string const& debug_name() const { return m_debug_name; }
#endif

  // Return the (next) queue for window_cookie (as passed to Application::create_root_window).
  vk::Queue acquire_queue(QueueFlags flags, int window_cookie);

  // Wait the completion of outstanding queue operations for all queues of this logical device.
  // This is a blocking call, only intended for program termination.
  void wait_idle() const { m_device->waitIdle(); }

 private:
  // Override this function to change the default physical device features.
  virtual void prepare_physical_device_features(PhysicalDeviceFeatures& physical_device_features) const { }

  // Override this function to add QueueRequest objects. The default will create a graphics and presentation queue.
  virtual void prepare_logical_device(DeviceCreateInfo& device_create_info) const { }

  // Override this function if you have QueueRequest's with overlapping QueueFlagBits.
  virtual QueueRequestIndex queue_index(QueueFlags flags, int window_cookie) const { return {}; }
};

} // namespace vulkan

namespace task {

class LogicalDevice : public AIStatefulTask
{
 private:
  vulkan::Application* m_application;
  task::VulkanWindow* m_root_window;                                    // The root window that we have to support presentation to (if any).
  std::unique_ptr<vulkan::LogicalDevice> m_logical_device;

 public:
  statefultask::TaskEvent m_logical_device_index_available_event;       // Triggered when m_root_window has its logical device index set.

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  /// The different states of the stateful task.
  enum logical_device_state_type {
    LogicalDevice_wait_for_window = direct_base_type::state_end,
    LogicalDevice_create,
    LogicalDevice_done
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = LogicalDevice_done + 1;
  static constexpr condition_type window_available_condition = 1;

 public:
  LogicalDevice(vulkan::Application* application COMMA_CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug)), m_application(application) { }

  void set_logical_device(std::unique_ptr<vulkan::LogicalDevice>&& logical_device) { m_logical_device = std::move(logical_device); }
  void set_root_window(task::VulkanWindow* root_window) { m_root_window = root_window; }

  int get_index() const { return m_root_window->get_logical_device_index(); }

 protected:
  /// Call finish() (or abort()), not delete.
  ~LogicalDevice() override = default;

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override final;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
