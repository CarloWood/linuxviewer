#include "sys.h"
#include "DispatchLoader.h"

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Storage space for the dynamic loader.
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#else // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Manually load VK_EXT_debug_utils extension functions.
// Copied from https://github.com/krOoze/Hello_Triangle/blob/master/src/DispatchLoader.h

#include <unordered_map>

// VK_EXT_debug_utils
//////////////////////////////////

namespace {

std::unordered_map<VkInstance, PFN_vkCreateDebugUtilsMessengerEXT> g_CreateDebugUtilsMessengerEXT_dispatch_table;
std::unordered_map<VkInstance, PFN_vkDestroyDebugUtilsMessengerEXT> g_DestroyDebugUtilsMessengerEXT_dispatch_table;
std::unordered_map<VkInstance, PFN_vkSubmitDebugUtilsMessageEXT> g_SubmitDebugUtilsMessageEXT_dispatch_table;
std::unordered_map<VkDevice, PFN_vkSetDebugUtilsObjectNameEXT> g_SetDebugUtilsObjectNameEXT_dispatch_table;

void load_DebugUtils_commands(VkInstance instance)
{
  PFN_vkVoidFunction temp_fp;

  temp_fp = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (!temp_fp) throw "Failed to load vkCreateDebugUtilsMessengerEXT";  // check shouldn't be necessary (based on spec)
  g_CreateDebugUtilsMessengerEXT_dispatch_table[instance] = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(temp_fp);

  temp_fp = vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (!temp_fp) throw "Failed to load vkDestroyDebugUtilsMessengerEXT";  // check shouldn't be necessary (based on spec)
  g_DestroyDebugUtilsMessengerEXT_dispatch_table[instance] = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(temp_fp);

  temp_fp = vkGetInstanceProcAddr(instance, "vkSubmitDebugUtilsMessageEXT");
  if (!temp_fp) throw "Failed to load vkSubmitDebugUtilsMessageEXT";  // check shouldn't be necessary (based on spec)
  g_SubmitDebugUtilsMessageEXT_dispatch_table[instance] = reinterpret_cast<PFN_vkSubmitDebugUtilsMessageEXT>(temp_fp);
}

void load_DebugUtils_commands(VkInstance instance, VkDevice device)
{
  PFN_vkVoidFunction temp_fp;

  temp_fp = vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
  if (!temp_fp) throw "Failed to load vkSetDebugUtilsObjectNameEXT";  // check shouldn't be necessary (based on spec)
  g_SetDebugUtilsObjectNameEXT_dispatch_table[device] = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(temp_fp);
}

void unload_DebugUtils_commands(VkInstance instance)
{
  g_CreateDebugUtilsMessengerEXT_dispatch_table.erase(instance);
  g_DestroyDebugUtilsMessengerEXT_dispatch_table.erase(instance);
  g_SubmitDebugUtilsMessageEXT_dispatch_table.erase(instance);
}

void unload_DebugUtils_commands(VkDevice device)
{
  g_SetDebugUtilsObjectNameEXT_dispatch_table.erase(device);
}

} // namespace

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerCreateInfoEXT const* pCreateInfo, VkAllocationCallbacks const* pAllocator, VkDebugUtilsMessengerEXT* pMessenger)
{
  auto dispatched_cmd = g_CreateDebugUtilsMessengerEXT_dispatch_table.at(instance);
  return dispatched_cmd(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const* pAllocator)
{
  auto dispatched_cmd = g_DestroyDebugUtilsMessengerEXT_dispatch_table.at(instance);
  return dispatched_cmd(instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData)
{
  auto dispatched_cmd = g_SubmitDebugUtilsMessageEXT_dispatch_table.at(instance);
  return dispatched_cmd(instance, messageSeverity, messageTypes, pCallbackData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT(VkDevice device, VkDebugUtilsObjectNameInfoEXT const* pNameInfo)
{
  auto dispatched_cmd = g_SetDebugUtilsObjectNameEXT_dispatch_table.at(device);
  return dispatched_cmd(device, pNameInfo);
}

namespace vulkan {

void DispatchLoader::load(vk::Instance vh_instance)
{
  load_DebugUtils_commands(vh_instance);
}

void DispatchLoader::load(vk::Instance vh_instance, vk::Device vh_device)
{
  load_DebugUtils_commands(vh_instance, vh_device);
}

} // namespace vulkan

#endif // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
