#pragma once

#include "DispatchLoader.h"
#include "VulkanWindow.h"
#include <boost/intrusive_ptr.hpp>

namespace vulkan {

class LogicalDevice
{
 public:
  virtual ~LogicalDevice() = default;

  void prepare(vk::Instance vulkan_instance, DispatchLoader& dispatch_loader, task::VulkanWindow const* window_task_ptr);
};

} // namespace vulkan
