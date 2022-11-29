#include "sys.h"
#include "TextureUpdateRequest.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void TextureUpdateRequest::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_texture:" << m_texture <<
      ", m_factory_characteristic_id:" << m_factory_characteristic_id <<
      ", m_subrange:" << m_subrange <<
      ", m_array_element_range:" << m_array_element_range;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
