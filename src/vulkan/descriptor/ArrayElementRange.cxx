#include "sys.h"
#include "ArrayElementRange.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void ArrayElementRange::print_on(std::ostream& os) const
{
  os << "{ArrayElementRange:[" << m_ibegin << ", " << m_iend << ">}";
}
#endif

} // namespace vulkan::descriptor
