#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

struct DeviceCreateInfo : vk::DeviceCreateInfo
{
 private:
   vk::QueueFlags m_queue_flags = vk::QueueFlagBits::eGraphics;         // Required queue flags, with default.

 public:
   // Setter for required queue flags.
   DeviceCreateInfo& setQueueFlags(vk::QueueFlags queue_flags)
   {
     m_queue_flags = queue_flags;
     return *this;
   }
};

} // namespace vulkan
