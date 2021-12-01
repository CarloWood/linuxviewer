#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>
#include <concepts>

namespace task {
class SynchronousWindow;
} // namespace task

namespace linuxviewer::OS {
class Window;
} // namespace linuxviewer::OS

namespace vulkan {

template<typename T>
concept ConceptWindowEvents = std::is_base_of_v<linuxviewer::OS::Window, T>;

template<typename T>
concept ConceptSynchronousWindow = std::is_base_of_v<task::SynchronousWindow, T>;

template<typename T>
concept ConceptVulkanHandle = vk::isVulkanHandleType<T>::value;

template<typename T>
struct is_unique_handle : std::false_type { };

template<typename T, typename Dispatch>
struct is_unique_handle<vk::UniqueHandle<T, Dispatch>> : std::true_type { };

template<typename T>
concept ConceptUniqueVulkanHandle = is_unique_handle<T>::value;

} // namespace vulkan
