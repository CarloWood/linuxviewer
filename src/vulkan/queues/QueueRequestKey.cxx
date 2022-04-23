#include "sys.h"
#include "QueueRequestKey.h"
#ifdef CWDEBUG
#include <iostream>
#endif

namespace vulkan {

#ifdef CWDEBUG
void QueueRequestKey::print_on(std::ostream& os) const
{
  os <<
    "{queue_flags:" << m_queue_flags <<
   ", request_cookie:0x" << std::hex << m_request_cookie << std::dec <<
   '}';
}
#endif

} // namespace vulkan
