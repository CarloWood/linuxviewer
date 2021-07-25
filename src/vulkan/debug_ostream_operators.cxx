#include "sys.h"

#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include <iostream>
#include <iomanip>
#include "debug.h"

namespace {

std::string print_version(uint32_t version)
{
  std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_MINOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_PATCH(version));
  return version_str;
}

} // namespace

using NAMESPACE_DEBUG::print_string;

std::ostream& operator<<(std::ostream& os, vk::ApplicationInfo const& application_info)
{
  os << '{';
  os << "allowDuplicate:" << std::boolalpha << application_info.allowDuplicate << ", ";
  os << "pApplicationName:" << print_string(application_info.pApplicationName) << ", ";
  os << "applicationVersion:" << print_version(application_info.applicationVersion) << ", ";
  os << "pEngineName:" << print_string(application_info.pEngineName) << ", ";
  os << "engineVersion:" << print_version(application_info.engineVersion) << ", ";
  os << "apiVersion:" << print_version(application_info.apiVersion) << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, vk::InstanceCreateInfo const& instance_create_info)
{
  os << '{';
  os << "flags:" << static_cast<decltype(instance_create_info.flags)::MaskType>(instance_create_info.flags) << ", ";
  os << "pApplicationInfo:" << (void*)instance_create_info.pApplicationInfo << ", ";
  os << "enabledLayerCount:" << instance_create_info.enabledLayerCount << ", ";
  os << "ppEnabledLayerNames:";
  for (int i = 0; i < instance_create_info.enabledLayerCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(instance_create_info.ppEnabledLayerNames[i]);
  }
  os << ", ";
  os << "enabledExtensionCount:" << instance_create_info.enabledExtensionCount << ", ";
  os << "ppEnabledExtensionNames:";
  for (int i = 0; i < instance_create_info.enabledExtensionCount; ++i)
  {
    if (i > 0)
      os << ',';
    os << print_string(instance_create_info.ppEnabledExtensionNames[i]);
  }
  os << '}';
  return os;
}

#endif // CWDEBUG
