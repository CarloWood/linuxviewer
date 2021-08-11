#include "sys.h"
#include "DeviceCreateInfo.h"
#include <cstring>
#ifdef CWDEBUG
#include "debug.h"
#include "debug_ostream_operators.h"
#include "cwds/debug_ostream_operators.h"
#endif

namespace vulkan {

DeviceCreateInfo& DeviceCreateInfo::addDeviceExtentions(vk::ArrayProxy<char const* const> extra_device_extensions)
{
  for (auto extra_name : extra_device_extensions)
  {
    bool found = false;
    for (auto name : m_device_extensions)
    {
      if (!std::strcmp(extra_name, name))
      {
        found = true;
        break;
      }
    }
    if (!found)
      m_device_extensions.push_back(extra_name);
  }
  // Update pointer and count in base class.
  setPEnabledExtensionNames(m_device_extensions);
  return *this;
}

#ifdef CWDEBUG
void DeviceCreateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << static_cast<vk::DeviceCreateInfo const&>(*this) << ", ";
  os << "m_queue_flags:" << m_queue_flags << ", ";
  os << "m_device_extensions:<";
  char const* prefix = "";
  for (char const* name : m_device_extensions)
  {
    os << prefix << debug::print_string(name);
    prefix = ", ";
  }
  os << ">}";
}
#endif

} // namespace vulkan
