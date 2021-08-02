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

} // namespace vk
