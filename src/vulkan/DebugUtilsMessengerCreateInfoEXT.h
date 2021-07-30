#pragma once

#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vulkan {

#ifdef CWDEBUG
VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData);
#endif

// Helper class.
struct DebugUtilsMessengerCreateInfoEXTArgs
{
  static constexpr vk::DebugUtilsMessageSeverityFlagsEXT default_messageSeverity =
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

  static constexpr vk::DebugUtilsMessageTypeFlagsEXT     default_messageType     =
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  static constexpr PFN_vkDebugUtilsMessengerCallbackEXT  default_pfnUserCallback =
#ifdef CWDEBUG
    debugCallback;
#else
    {};
#endif

  vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = default_messageSeverity;
  vk::DebugUtilsMessageTypeFlagsEXT     messageType     = default_messageType;
  PFN_vkDebugUtilsMessengerCallbackEXT  pfnUserCallback = default_pfnUserCallback;
  void *                                pUserData       = {};
};

struct DebugUtilsMessengerCreateInfoEXT : public vk::DebugUtilsMessengerCreateInfoEXT
{
  DebugUtilsMessengerCreateInfoEXT(DebugUtilsMessengerCreateInfoEXTArgs&& = {});

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
