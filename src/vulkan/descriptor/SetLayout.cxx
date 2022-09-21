#include "sys.h"
#include "SetLayout.h"
#include "LogicalDevice.h"

namespace vulkan::descriptor {

void SetLayout::realize_handle(LogicalDevice const* logical_device)
{
  // Get cached or new vk::DescriptorSetLayout handle.
  m_handle = logical_device->realize_descriptor_set_layout(m_sorted_bindings);
}

} // namespace vulkan::descriptor
