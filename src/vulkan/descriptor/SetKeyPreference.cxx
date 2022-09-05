#include "sys.h"
#include "SetKeyPreference.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetKeyPreference::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_set_key:" << m_set_key <<
      ", m_preference:" << m_preference; 
  os << '}';
}
#endif

} // namespace vulkan::descriptor
