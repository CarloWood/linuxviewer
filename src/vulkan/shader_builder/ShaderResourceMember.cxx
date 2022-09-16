#include "sys.h"
#include "ShaderResourceMember.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_pointer.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderResourceMember::print_on(std::ostream& os) const
{
  os << '{';
  os << "GlslIdFull:" << NAMESPACE_DEBUG::print_string(glsl_id_full()) <<
      ", m_member_index:" << m_member_index <<
      ", m_basic_type:" << m_basic_type <<
      ", m_offset:" << m_offset <<
      ", m_array_size:" << m_array_size;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
