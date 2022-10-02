#include "sys.h"
#include "Base.h"
#ifdef CWDEBUG
#include <iomanip>
#endif
#include "debug.h"

namespace vulkan::shader_builder::shader_resource {

#ifdef CWDEBUG
void Base::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_created_and_updated_descriptor_set:" << std::boolalpha << m_created_and_updated_descriptor_set.load(std::memory_order::relaxed) <<
      ", m_descriptor_set_key:" << m_descriptor_set_key <<
      ", m_set_layout_bindings:";
  os << *set_layout_bindings_t::crat{m_set_layout_bindings};
  os << ", object_name:" << m_ambifix.object_name();
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
