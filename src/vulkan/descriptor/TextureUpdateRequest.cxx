#include "sys.h"
#include "TextureUpdateRequest.h"
#include "debug.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void TextureUpdateRequest::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_factory_characteristic_id:" << m_factory_characteristic_id <<
      ", m_texture:" << m_texture;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
