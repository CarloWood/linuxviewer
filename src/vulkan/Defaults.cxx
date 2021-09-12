#include "sys.h"
#include "Defaults.h"

#ifdef CWDEBUG
#include "utils/PrintList.h"
#include "utils/print_version.h"
#include "debug.h"
#include <iomanip>

namespace vk_defaults {

using vk_utils::print_version;
using vk_utils::print_list;
using NAMESPACE_DEBUG::print_string;

void PrintOn::print_on(std::ostream& os) const
{
  os << '{';
  print_members(os, "");
  os << '}';
}

void ApplicationInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
      "allowDuplicate:"          << std::boolalpha << allowDuplicate <<
    ", pApplicationName:"        << print_string(pApplicationName) <<
    ", applicationVersion:"      << print_version(applicationVersion) <<
    ", pEngineName:"             << print_string(pEngineName) <<
    ", engineVersion:"           << print_version(engineVersion) <<
    ", apiVersion:"              << print_version(apiVersion);
}

void InstanceCreateInfo::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
      "flags:"                   << flags <<
    ", pApplicationInfo:"        << (void*)pApplicationInfo;

  if (pApplicationInfo)
    os << " (" << *pApplicationInfo << ')';

  os <<
    ", enabledLayerCount:"       << enabledLayerCount <<
    ", ppEnabledLayerNames:"     << print_list(ppEnabledLayerNames, enabledLayerCount) <<
    ", enabledExtensionCount:"   << enabledExtensionCount <<
    ", ppEnabledExtensionNames:" << print_list(ppEnabledExtensionNames, enabledExtensionCount);
}

void DebugUtilsMessengerCreateInfoEXT::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix;

  if (pNext)
    os << "pNext:" << pNext << ", ";

  os <<
      "flags:"                   << flags <<
    ", messageSeverity:"         << messageSeverity <<
    ", messageType:"             << messageType <<
    ", pfnUserCallback:"         << pfnUserCallback <<
    ", pUserData:"               << pUserData;
}

void Instance::print_members(std::ostream& os, char const* prefix) const
{
  os << prefix <<
    "m_instance: " << static_cast<VkInstance>(*this);
}

} // namespace vk_defaults

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
channel_ct vkverbose("VKVERBOSE");
channel_ct vkinfo("VKINFO");
channel_ct vkwarning("VKWARNING");
channel_ct vkerror("VKERROR");
NAMESPACE_DEBUG_CHANNELS_END
#endif

#endif
