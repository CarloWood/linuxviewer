#include "sys.h"
#include "BasicType.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void BasicType::print_on(std::ostream& os) const
{
  os << '{';
  if (m_rows == 0 && m_cols == 0)
    os << "<empty BasicType>";
  else
    os << "m_standard:" << to_string(standard()) <<
        ", m_rows:" << m_rows <<
        ", m_cols:" << m_cols <<
        ", m_scalar_type:" << scalar_type() <<
        ", m_log2_alignment:" << m_log2_alignment <<
        ", m_size:" << m_size <<
        ", m_array_stride:" << m_array_stride;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
