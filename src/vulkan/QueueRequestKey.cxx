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
    "{queue_flags:" << queue_flags <<
   ", cookie:" << cookie <<
   '}';
}
#endif

} // namespace vulkan
