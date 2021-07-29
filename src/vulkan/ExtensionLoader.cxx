#include "sys.h"
#include "ExtensionLoader.h"

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Storage space for the dynamic loader.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#else // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Manually load VK_EXT_debug_utils extension functions.
// Copied from https://github.com/krOoze/Hello_Triangle/blob/master/src/ExtensionLoader.h

#include <unordered_map>

// VK_EXT_debug_utils
//////////////////////////////////

namespace {

std::unordered_map<VkInstance, PFN_vkCreateDebugUtilsMessengerEXT> CreateDebugUtilsMessengerEXTDispatchTable;
std::unordered_map<VkInstance, PFN_vkDestroyDebugUtilsMessengerEXT> DestroyDebugUtilsMessengerEXTDispatchTable;
std::unordered_map<VkInstance, PFN_vkSubmitDebugUtilsMessageEXT> SubmitDebugUtilsMessageEXTDispatchTable;

void loadDebugUtilsCommands(VkInstance instance)
{
  PFN_vkVoidFunction temp_fp;

  temp_fp = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (!temp_fp) throw "Failed to load vkCreateDebugUtilsMessengerEXT";  // check shouldn't be necessary (based on spec)
  CreateDebugUtilsMessengerEXTDispatchTable[instance] = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(temp_fp);

  temp_fp = vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (!temp_fp) throw "Failed to load vkDestroyDebugUtilsMessengerEXT";  // check shouldn't be necessary (based on spec)
  DestroyDebugUtilsMessengerEXTDispatchTable[instance] = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(temp_fp);

  temp_fp = vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
  if (!temp_fp) throw "Failed to load vkSubmitDebugUtilsMessageEXT";  // check shouldn't be necessary (based on spec)
  SubmitDebugUtilsMessageEXTDispatchTable[instance] = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(temp_fp);
}

void unloadDebugUtilsCommands(VkInstance instance)
{
  CreateDebugUtilsMessengerEXTDispatchTable.erase(instance);
  DestroyDebugUtilsMessengerEXTDispatchTable.erase(instance);
  SubmitDebugUtilsMessageEXTDispatchTable.erase(instance);
}

} // namespace

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
  auto dispatched_cmd = CreateDebugUtilsMessengerEXTDispatchTable.at(instance);
  return dispatched_cmd(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator)
{
  auto dispatched_cmd = DestroyDebugUtilsMessengerEXTDispatchTable.at(instance);
  return dispatched_cmd(instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
  auto dispatched_cmd = SubmitDebugUtilsMessageEXTDispatchTable.at(instance);
  return dispatched_cmd(instance, messageSeverity, messageTypes, pCallbackData);
}

namespace vulkan {

void ExtensionLoader::setup(vk::Instance vulkan_instance)
{
  loadDebugUtilsCommands(vulkan_instance);
}

} // namespace vulkan

#endif // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
