#pragma once

#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vk {

std::ostream& operator<<(std::ostream& os, vk::ApplicationInfo const& application_info);
std::ostream& operator<<(std::ostream& os, vk::InstanceCreateInfo const& instance_create_info);
std::ostream& operator<<(std::ostream& os, vk::DebugUtilsObjectNameInfoEXT const& debug_utils_object_name_info);

} // namespace vk
