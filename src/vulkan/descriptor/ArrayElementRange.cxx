#include "sys.h"
#include "ArrayElementRange.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void ArrayElementRange::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_ibegin:" << m_ibegin <<
      ", m_iend:" << m_iend;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
