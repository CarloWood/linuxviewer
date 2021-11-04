#include "sys.h"

#ifdef CWDEBUG
#include "Defaults.h"
#include "DebugUtilsMessenger.h"
#include "debug.h"

namespace vulkan {

void DebugUtilsMessenger::prepare(vk::Instance vh_instance, vk::DebugUtilsMessengerCreateInfoEXT const& debug_create_info)
{
  // Keep a copy, because we need that in the destructor.
  m_vh_instance = vh_instance;
  m_debug_utils_messenger = m_vh_instance.createDebugUtilsMessengerEXTUnique(debug_create_info);
}

// Default callback function for debug output from vulkan layers.
//static
VkBool32 DebugUtilsMessenger::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* UNUSED_ARG(user_data))
{
  char const* color_end = "";
  libcwd::channel_ct* dcp;
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    dcp = &DEBUGCHANNELS::dc::vkerror;
    Dout(dc::vkerror|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
    color_end = "\e[0m";
#ifdef CWDEBUG
    if (NAMESPACE_DEBUG::being_traced())
      DoutFatal(dc::core, "Trap point");
#endif
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    dcp = &DEBUGCHANNELS::dc::vkwarning;
    Dout(dc::vkwarning|dc::warning|continued_cf, "\e[31m" << pCallbackData->pMessage);
    color_end = "\e[0m";
  }
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    dcp = &DEBUGCHANNELS::dc::vkinfo;
    Dout(dc::vkinfo|continued_cf, pCallbackData->pMessage);
  }
  else
  {
    dcp = &DEBUGCHANNELS::dc::vkverbose;
    Dout(dc::vkverbose|continued_cf, pCallbackData->pMessage);
  }

  if (pCallbackData->objectCount > 0)
  {
    Dout(dc::continued, " [with an objectCount of " << pCallbackData->objectCount << "]");
    for (int i = 0; i < pCallbackData->objectCount; ++i)
      Dout(*dcp, static_cast<vk_defaults::DebugUtilsObjectNameInfoEXT>(pCallbackData->pObjects[i]));
  }

  Dout(dc::finish, color_end);

  return VK_FALSE;
}

} // namespace vulkan

#endif
