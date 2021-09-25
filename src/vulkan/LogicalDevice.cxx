#include "sys.h"
#include "LogicalDevice.h"

namespace vulkan {

void LogicalDevice::prepare(vk::Instance vulkan_instance, DispatchLoader& dispatch_loader, task::VulkanWindow const* window_task_ptr)
{
  DoutEntering(dc::vulkan, "vulkan::LogicalDevice::prepare(" << vulkan_instance << ", dispatch_loader, " << (void*)window_task_ptr << ")");

  boost::intrusive_ptr<task::VulkanWindow const> root_window(window_task_ptr);
}

} // namespace vulkan
