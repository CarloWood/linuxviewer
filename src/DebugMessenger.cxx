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

VkResult CreateDebugUtilsMessengerEXT(
    vk::Instance instance,
    VkDebugUtilsMessengerCreateInfoEXT const* pCreateInfo,
    VkAllocationCallbacks const* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func)
    return VK_ERROR_EXTENSION_NOT_PRESENT;

  return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
  createInfo                 = {};
  createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData       = nullptr;  // Optional
}

void DestroyDebugUtilsMessengerEXT(vk::Instance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks const* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) { func(instance, debugMessenger, pAllocator); }
}

} // namespace

void DebugMessenger::setup(vk::Instance vulkan_instance)
{
  // Keep a copy, because we need that in the destructor.
  m_vulkan_instance = vulkan_instance;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);
  if (CreateDebugUtilsMessengerEXT(m_vulkan_instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    throw std::runtime_error("failed to set up debug messenger!");
}

DebugMessenger::~DebugMessenger()
{
  DestroyDebugUtilsMessengerEXT(m_vulkan_instance, debugMessenger, nullptr);
}
#endif
