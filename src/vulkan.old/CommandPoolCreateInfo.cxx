#include "sys.h"
#include "CommandPoolCreateInfo.h"
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include <iostream>
#endif

namespace vulkan {

#ifdef CWDEBUG
void CommandPoolCreateInfo::print_on(std::ostream& os) const
{
  os << '{';
  os << "pNext:" << pNext << ", ";
  os << "flags:" << flags << ", ";
  os << "queueFamilyIndex:" << queueFamilyIndex;
  os << '}';
}
#endif

} // namespace vulkan
