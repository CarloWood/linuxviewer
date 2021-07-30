#include "sys.h"

#ifdef CWDEBUG
#include "DebugUtilsMessengerCreateInfoEXT.h"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include "debug.h"

// The operator<<'s must be declared in namespace vk (where DebugUtilsMessageSeverityFlagBitsEXT
// and DebugUtilsMessageTypeFlagBitsEXT are defined) so that they will be found with ADL.
namespace vk {

std::ostream& operator<<(std::ostream& os, DebugUtilsMessageSeverityFlagBitsEXT const& severity)
{
  return os << to_string(severity);
}

std::ostream& operator<<(std::ostream& os, DebugUtilsMessageTypeFlagBitsEXT const& type)
{
  return os << to_string(type);
}

} // namespace vk

namespace vulkan {

// Callback function for debug output from vulkan layers.
VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData)
{
  Dout(dc::vulkan|dc::warning, pCallbackData->pMessage);
  return VK_FALSE;
}

DebugUtilsMessengerCreateInfoEXT::DebugUtilsMessengerCreateInfoEXT(DebugUtilsMessengerCreateInfoEXTArgs&& args) :
  vk::DebugUtilsMessengerCreateInfoEXT(
      {},                       // Reserved for future use (should be zero).
      args.messageSeverity,     // Is a bitmask of vk::DebugUtilsMessageSeverityFlagsEXT specifying which severity of event(s) will cause this callback to be called.
      args.messageType,         // Is a bitmask of vk::DebugUtilsMessageTypeFlagsEXT specifying which type of event(s) will cause this callback to be called.
      args.pfnUserCallback,     // Is the application callback function to call.
      args.pUserData)           // Is user data to be passed to the callback.
{
}

void DebugUtilsMessengerCreateInfoEXT::print_on(std::ostream& os) const
{
  os << '{';
  os << "messageSeverity:" << debug::print_flags<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity) << ", ";
  os << "messageType:" << debug::print_flags<vk::DebugUtilsMessageTypeFlagBitsEXT>(messageType) << ", ";
  os << "pfnUserCallback:";
  if (!pfnUserCallback)
    os << "nullptr";    // This is an error, the validation layer should complain.
  else
  {
    void const* user_callback = reinterpret_cast<void const*>(pfnUserCallback);
    os <<
#if CWDEBUG_LOCATION
      libcwd::pc_mangled_function_name(user_callback);
#else
      user_callback;
#endif
  }
  os << ", ";
  os << "pUserData:" << pUserData;
  os << '}';
}

} // namespace vulkan
#endif
