#pragma once

#include "PhysicalDeviceFeatures.h"
#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vulkan {

struct DeviceCreateInfo : vk::DeviceCreateInfo
{
 private:
   vk::QueueFlags m_queue_flags = vk::QueueFlagBits::eGraphics;         // Required queue flags, with default.
   bool m_presentation = true;                                          // Whether or not presentation capability (for the surface passed to Device::setup) is required.
   std::vector<char const*> m_device_extensions;

 public:
   DeviceCreateInfo(vulkan::PhysicalDeviceFeatures const& physical_device_features = vulkan::PhysicalDeviceFeatures::s_default_physical_device_features)
   {
     setPEnabledFeatures(&physical_device_features);
   }

   // Setter for required queue flags.
   DeviceCreateInfo& setQueueFlags(vk::QueueFlags queue_flags)
   {
     m_queue_flags = queue_flags;
     return *this;
   }

   // Setter for presentation flag.
   DeviceCreateInfo& setPresentationFlag(bool need_presentation)
   {
     m_presentation = need_presentation;
     return *this;
   }

   void addDeviceExtentions(vk::ArrayProxy<char const* const> extra_device_extensions);

   bool has_queue_flag(vk::QueueFlagBits queue_flag) const
   {
     return !!(m_queue_flags & queue_flag);
   }

   vk::QueueFlags get_queue_flags() const
   {
     return m_queue_flags;
   }

   bool get_presentation_flag() const
   {
     return m_presentation;
   }

#ifdef CWDEBUG
   void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
