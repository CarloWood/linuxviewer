#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>
#include <concepts>

namespace task {
class VulkanWindow;
} // namespace task

namespace vulkan {

template<typename T>
concept ConceptVulkanWindow = std::derived_from<T, task::VulkanWindow>;

template<typename T>
concept ConceptVulkanHandle = vk::isVulkanHandleType<T>::value;

template<typename T>
struct is_unique_handle : std::false_type { };

template<typename T, typename Dispatch>
struct is_unique_handle<vk::UniqueHandle<T, Dispatch>> : std::true_type { };

template<typename T>
concept ConceptUniqueVulkanHandle = is_unique_handle<T>::value;

} // namespace vulkan
