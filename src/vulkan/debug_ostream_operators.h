#pragma once

#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vk {

// ostream inserters for objects in namespace vk:
std::ostream& operator<<(std::ostream& os, ApplicationInfo const& application_info);
std::ostream& operator<<(std::ostream& os, InstanceCreateInfo const& instance_create_info);
std::ostream& operator<<(std::ostream& os, DebugUtilsObjectNameInfoEXT const& debug_utils_object_name_info);

template<typename EnumBits>
std::ostream& operator<<(std::ostream& os, Flags<EnumBits> en)
{
  return os << to_string(en);
}

std::ostream& operator<<(std::ostream& os, DeviceCreateInfo const& device_create_info);
std::ostream& operator<<(std::ostream& os, DeviceQueueCreateInfo const& device_queue_create_info);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceFeatures const& physical_device_features);
std::ostream& operator<<(std::ostream& os, QueueFamilyProperties const& queue_family_properties);
std::ostream& operator<<(std::ostream& os, Extent3D const& extend_3D);
std::ostream& operator<<(std::ostream& os, QueueFlagBits const& queue_flag_bit);
std::ostream& operator<<(std::ostream& os, ExtensionProperties const& extension_properties);
std::ostream& operator<<(std::ostream& os, PhysicalDeviceProperties const& physical_device_properties);

} // namespace vk
