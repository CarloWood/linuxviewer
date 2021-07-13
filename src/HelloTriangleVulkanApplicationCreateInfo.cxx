#include "sys.h"

#ifdef CWDEBUG
#include "HelloTriangleVulkanApplicationCreateInfo.h"
#include <iostream>

void HelloTriangleVulkanApplicationCreateInfo::print_on(std::ostream& os) const
{
  static_cast<ApplicationCreateInfo const*>(this)->print_on(os);
  os << ", {";
  os << "version:" << version;
  os << '}';
}
#endif

