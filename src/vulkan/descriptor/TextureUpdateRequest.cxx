#include "sys.h"
#include "TextureUpdateRequest.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void TextureUpdateRequest::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_texture_array_range:" << m_texture_array_range <<
      ", m_factory_characteristic_id:" << m_factory_characteristic_id <<
      ", m_subrange:" << m_subrange;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
