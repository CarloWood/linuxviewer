#include "sys.h"
#include "CommandBuffer.h"
#include <iostream>

namespace vulkan::handle {

#ifdef CWDEBUG
void CommandBuffer::print_on(std::ostream& os) const
{
  os << '{';
  os << static_cast<vk::CommandBuffer const&>(*this);
  os << '}';
}
#endif

} // namespace vulkan::handle
