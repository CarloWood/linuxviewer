#include "sys.h"
#include "ConsecutiveRange.h"
#ifdef CWDEBUG
#include <iostream>
#endif

namespace vulkan::pipeline {

#ifdef CWDEBUG
void ConsecutiveRange::print_on(std::ostream& os) const
{
  os << "{ConsecutiveRange:[" << m_begin << ", " << m_end << ">}";
}
#endif

} // namespace vulkan::pipeline
