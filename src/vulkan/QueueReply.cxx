#include "sys.h"
#include "QueueReply.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

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
  os << ", m_acquired:" << m_acquired << ", ";
  os << "m_window_cookies:0x" << std::hex << m_window_cookies << std::dec;
  os << '}';
}
#endif

} // namespace vulkan
