#include "sys.h"
#include "ProvidesUpdate.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void ProvidesUpdate::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_texture:" << m_texture <<
      ", m_factory_range_id:" << m_factory_range_id;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
