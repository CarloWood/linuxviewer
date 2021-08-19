#include "sys.h"

#ifdef CWDEBUG
#include "Queue.h"
#include "debug_ostream_operators.h"
#endif

namespace vulkan {

#ifdef CWDEBUG
void Queue::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_queue_family:" << m_queue_family << ", ";
  os << "m_queue:" << m_queue;
  os << '}';
}
#endif

} // namespace vulkan
