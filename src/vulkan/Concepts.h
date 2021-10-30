#pragma once

#include <type_traits>
#include <concepts>

namespace task {
class VulkanWindow;
} // namespace task

namespace vulkan {

template<typename T>
concept ConceptVulkanWindow = std::derived_from<T, task::VulkanWindow>;

} // namespace vulkan
