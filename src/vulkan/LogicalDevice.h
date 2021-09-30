#pragma once

#include "DispatchLoader.h"
#include "VulkanWindow.h"
#include <boost/intrusive_ptr.hpp>

namespace vulkan {

class PhysicalDeviceFeatures;
class DeviceCreateInfo;

class LogicalDevice
{
 public:
  virtual ~LogicalDevice() = default;

  void prepare(vk::Instance vulkan_instance, DispatchLoader& dispatch_loader, task::VulkanWindow const* window_task_ptr);

 private:
  // Override this function to change the default physical device features.
  virtual void prepare_physical_device_features(PhysicalDeviceFeatures& physical_device_features) const { }

  // Override this function to add QueueRequest objects. The default will create a graphics and presentation queue.
  virtual void prepare_logical_device(DeviceCreateInfo& device_create_info) const { }
};

} // namespace vulkan

namespace task {

class LogicalDevice : public AIStatefulTask
{
 private:
  vulkan::Application* m_application;
  task::VulkanWindow const* m_root_window;
  std::unique_ptr<vulkan::LogicalDevice> m_logical_device;

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
  void set_root_window(task::VulkanWindow const* root_window) { m_root_window = root_window; }

 protected:
  /// Call finish() (or abort()), not delete.
  ~LogicalDevice() override = default;

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override final;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
