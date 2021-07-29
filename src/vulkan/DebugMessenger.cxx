#include "sys.h"

#ifdef CWDEBUG
#include "DebugMessenger.h"
#include <vulkan/vulkan_core.h>
#include "debug.h"

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace {

// Local callback function for debug output from vulkan layers.
VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData)
{
  Dout(dc::vulkan|dc::warning, pCallbackData->pMessage);
  return VK_FALSE;
}

} // namespace

namespace vulkan {

void DebugMessenger::setup(vk::Instance vulkan_instance)
{
  // Keep a copy, because we need that in the destructor.
  m_vulkan_instance = vulkan_instance;

  vk::DebugUtilsMessengerCreateInfoEXT createInfo;
  createInfo
    .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
    .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
    .setPfnUserCallback(debugCallback)
    ;

  m_debug_messenger = m_vulkan_instance.createDebugUtilsMessengerEXTUnique(createInfo);
}

} // namespace vulkan
#endif
