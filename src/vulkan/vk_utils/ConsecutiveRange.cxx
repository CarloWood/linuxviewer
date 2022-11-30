#include "sys.h"
#include "ConsecutiveRange.h"
#ifdef CWDEBUG
#include <iostream>
#endif

namespace vk_utils {

#ifdef CWDEBUG
void ConsecutiveRange::print_on(std::ostream& os) const
{
  os << "{ConsecutiveRange:[" << m_begin << ", " << m_end << ">}";
}
#endif

} // namespace vk_utils
