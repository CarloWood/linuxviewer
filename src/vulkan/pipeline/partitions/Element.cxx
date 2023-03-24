#include "sys.h"
#include "Element.h"
#include <iostream>

namespace vulkan::pipeline::partitions {

#ifdef CWDEBUG
void Element::print_on(std::ostream& os) const
{
  os << m_name;
}
#endif // CWDEBUG

} // namespace vulkan::pipeline::partitions
