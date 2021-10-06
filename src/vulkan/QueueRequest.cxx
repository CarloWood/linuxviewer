#include "sys.h"
#include "QueueRequest.h"
#include "debug.h"
#ifdef CWDEBUG
#include "cwds/debug_ostream_operators.h"
#endif

namespace vulkan {

#ifdef CWDEBUG
void QueueRequest::print_on(std::ostream& os) const
{
  os << '{';
  os << "queue_flags:" << queue_flags << ", ";
  os << "max_number_of_queues:" << max_number_of_queues << ", ";
  os << "priority:" << priority;
  if (!combined_with.undefined())
    os << ", combined_with:" << combined_with;
  os << '}';
}
#endif

} // namespace vulkan
