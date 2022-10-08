#include "sys.h"
#include "SetLayoutBinding.h"
#ifdef CWDEBUG
#include "SetLayout.h"
#include "vk_utils/print_pointer.h"
#endif

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetLayoutBinding::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_set_layout:" << m_descriptor_set_layout <<
      ", m_binding:" << m_binding;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
