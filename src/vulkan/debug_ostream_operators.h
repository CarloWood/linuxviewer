#pragma once

#include <vulkan/vulkan.hpp>
#include <iosfwd>

std::ostream& operator<<(std::ostream& os, vk::ApplicationInfo const& application_info);
std::ostream& operator<<(std::ostream& os, vk::InstanceCreateInfo const& instance_create_info);
