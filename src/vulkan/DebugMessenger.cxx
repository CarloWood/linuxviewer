#include "sys.h"

#ifdef CWDEBUG
#include "DebugMessenger.h"
#include <vulkan/vulkan_core.h>
#include "debug.h"

namespace vulkan {

// Default callback function for debug output from vulkan layers.
VkBool32 DebugMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData)
{
  Dout(dc::vulkan|dc::warning, pCallbackData->pMessage);
  return VK_FALSE;
}

void DebugMessenger::setup(vk::Instance vulkan_instance, vk::DebugUtilsMessengerCreateInfoEXT const& debug_create_info)
{
  // Keep a copy, because we need that in the destructor.
  m_vulkan_instance = vulkan_instance;
  m_debug_messenger = m_vulkan_instance.createDebugUtilsMessengerEXTUnique(debug_create_info);
}

} // namespace vulkan
#endif
