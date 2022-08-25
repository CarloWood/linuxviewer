#include "sys.h"
#include "Pipeline.h"

namespace vulkan {

#ifdef CWDEBUG
void Pipeline::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_vh_layout:" << m_vh_layout <<
      ", m_handle:" << m_handle;
  os << '}';
}
#endif

} // namespace vulkan
