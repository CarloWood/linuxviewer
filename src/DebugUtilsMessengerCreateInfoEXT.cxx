#include "sys.h"
#include "DebugUtilsMessengerCreateInfoEXT.h"
#include "vulkan/DebugMessenger.h"
#include "Application.h"
#include "debug.h"

#ifdef CWDEBUG
#include <iostream>

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
#endif // CWDEBUG

#if 0
DebugUtilsMessengerCreateInfoEXT::DebugUtilsMessengerCreateInfoEXT(DebugUtilsMessengerCreateInfoEXTArgs&& args) :
  vk::DebugUtilsMessengerCreateInfoEXT(
      {},                       // Reserved for future use (should be zero).
      args.messageSeverity,     // Is a bitmask of vk::DebugUtilsMessageSeverityFlagsEXT specifying which severity of event(s) will cause this callback to be called.
      args.messageType,         // Is a bitmask of vk::DebugUtilsMessageTypeFlagsEXT specifying which type of event(s) will cause this callback to be called.
      args.pfnUserCallback,     // Is the application callback function to call.
      args.pUserData)           // Is user data to be passed to the callback.
{
}
#endif

DebugUtilsMessengerCreateInfoEXT::DebugUtilsMessengerCreateInfoEXT(Application& application)
{
#ifdef CWDEBUG
  pfnUserCallback = &vulkan::DebugMessenger::debugCallback;
  pUserData = application.get_debug_messenger_ptr();
#endif
}

#ifdef CWDEBUG
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
#endif // CWDEBUG
