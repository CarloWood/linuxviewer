#include "sys.h"
#include "SetBinding.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetBinding::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_set_index_hint:" << m_set_index_hint <<
      ", m_binding:" << m_binding;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
