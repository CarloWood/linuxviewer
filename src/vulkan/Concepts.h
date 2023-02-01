#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>
#include <concepts>

class AIStatefulTask;

namespace linuxviewer::OS {
class Window;
} // namespace linuxviewer::OS

namespace vulkan {

namespace task {
class PipelineFactory;
class SynchronousWindow;
} // namespace task

namespace pipeline {
class CharacteristicRange;
} // namespace pipeline

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

template<typename T>
concept ConceptPipelineCharacteristic = std::is_base_of_v<pipeline::CharacteristicRange, T>;

template<typename T>
concept ConceptWriteDescriptorSetUpdateInfo =
  std::is_same_v<T, std::array<vk::DescriptorImageInfo, 1>> ||
  std::is_same_v<T, std::array<vk::DescriptorBufferInfo, 1>> ||
  std::is_same_v<T, std::array<vk::BufferView, 1>> ||
  std::is_same_v<T, std::vector<vk::DescriptorImageInfo>> ||
  std::is_same_v<T, std::vector<vk::DescriptorBufferInfo>> ||
  std::is_same_v<T, std::vector<vk::BufferView>>;

} // namespace vulkan
