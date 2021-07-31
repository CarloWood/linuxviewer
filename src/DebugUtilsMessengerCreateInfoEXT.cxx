#include "sys.h"
#include "DebugUtilsMessengerCreateInfoEXT.h"
#include "vulkan/DebugMessenger.h"
#include "Application.h"
#include "debug.h"

#ifdef CWDEBUG
#include <iostream>

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vkverbose;
extern channel_ct vkinfo;
extern channel_ct vkwarning;
extern channel_ct vkerror;
NAMESPACE_DEBUG_CHANNELS_END
#endif

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

DebugUtilsMessengerCreateInfoEXT::DebugUtilsMessengerCreateInfoEXT(Application& application)
{
#ifdef CWDEBUG
  // Assign the default values.
  messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
  messageType = default_messageType;
  pfnUserCallback = &Application::debugCallback;
  pUserData = &application;

  // Also turn on severity bits corresponding to debug channels that are on.
  if (DEBUGCHANNELS::dc::vkwarning.is_on())
    messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
  if (DEBUGCHANNELS::dc::vkinfo.is_on())
    messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
  if (DEBUGCHANNELS::dc::vkverbose.is_on())
    messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
#endif
}
