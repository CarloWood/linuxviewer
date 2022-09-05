#include "sys.h"
#include "UniformBuffer.h"
#include "debug.h"

namespace vulkan::memory {

#ifdef CWDEBUG
void UniformBuffer::print_on(std::ostream& os) const
{
  os << '{';
  os << "Buffer:";
  Buffer::print_on(os);
  os << ", m_pointer:" << m_pointer;
  os << '}';
}
#endif

} // namespace vulkan::memory
