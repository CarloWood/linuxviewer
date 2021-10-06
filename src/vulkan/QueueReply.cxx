#include "sys.h"
#include "QueueReply.h"

namespace vulkan {

#ifdef CWDEBUG
void QueueReply::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_queue_family:" << m_queue_family << ", ";
  os << "m_number_of_queues:" << m_number_of_queues << ", ";
  os << "m_requested_queue_flags:" << m_requested_queue_flags << ", ";
  if (m_combined_with.undefined())
    os << "m_start_index:" << m_start_index;
  else
    os << "m_combined_with:" << m_combined_with;
  os << '}';
}
#endif

} // namespace vulkan
