#pragma once

#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan {

class DebugUtilsMessenger
{
 private:
  vk::Instance m_vh_instance;                                   // Copy of the instance that was passed to prepare.
  vk::UniqueDebugUtilsMessengerEXT m_debug_utils_messenger;     // The unlaying debug messenger.

 public:
  DebugUtilsMessenger() = default;

  // The default debug callback for debug messages from vulkan.
  static VkBool32 debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
      void* pUserData);

  // The life time of the vulkan instance that is passed to prepare must be longer
  // than the life time of this object. The reason for that is that the destructor
  // of this object uses it.
  void prepare(vk::Instance vh_instance, vk::DebugUtilsMessengerCreateInfoEXT const& create_info);
};

} // namespace vulkan
