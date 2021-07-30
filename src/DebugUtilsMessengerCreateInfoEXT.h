#pragma once

#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include <iosfwd>
#endif

#if 0
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
    &DebugMessenger::debugCallback;
#else
    {};
#endif

  vk::DebugUtilsMessageSeverityFlagsEXT messageSeverity = default_messageSeverity;
  vk::DebugUtilsMessageTypeFlagsEXT     messageType     = default_messageType;
  PFN_vkDebugUtilsMessengerCallbackEXT  pfnUserCallback = default_pfnUserCallback;
  void *                                pUserData       = {};
};
#endif

class Application;

struct DebugUtilsMessengerCreateInfoEXT : public vk::DebugUtilsMessengerCreateInfoEXT
{
  DebugUtilsMessengerCreateInfoEXT(Application& application);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
