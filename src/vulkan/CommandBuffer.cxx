#include "sys.h"
#include "CommandBuffer.h"
#include <iostream>

namespace vulkan::handle {

#ifdef CWDEBUG
void CommandBuffer::print_on(std::ostream& os) const
{
  os << "{vh_command_buffer:" << m_vh_command_buffer << '}';
}
#endif

} // namespace vulkan::handle
