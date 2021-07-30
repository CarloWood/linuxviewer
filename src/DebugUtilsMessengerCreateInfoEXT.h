#pragma once

#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include <iosfwd>
#endif

class Application;

struct DebugUtilsMessengerCreateInfoEXT : public vk::DebugUtilsMessengerCreateInfoEXT
{
  static constexpr vk::DebugUtilsMessageSeverityFlagsEXT default_messageSeverity =
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
    vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

  static constexpr vk::DebugUtilsMessageTypeFlagsEXT     default_messageType     =
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

  DebugUtilsMessengerCreateInfoEXT(Application& application);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
