#include "sys.h"
#include "TextureArrayRange.h"
#include "debug.h"

namespace vulkan {

#ifdef CWDEBUG
void TextureArrayRange::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_texture_array:" << m_texture_array <<
      ", m_array_element_range:" << m_array_element_range;
  os << '}';
}
#endif

} // namespace vulkan
