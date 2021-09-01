#include "sys.h"
#include "QueueReply.h"

namespace vulkan {

#ifdef CWDEBUG
void QueueReply::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_queue_family_handle:" << m_queue_family_handle << ", ";
  os << "m_number_of_queues:" << m_number_of_queues << ", ";
  os << '}';
}
#endif

} // namespace vulkan
