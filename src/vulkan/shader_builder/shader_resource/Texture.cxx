#include "sys.h"
#include "Texture.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#include "utils/has_print_on.h"
#endif

namespace vulkan::shader_builder::shader_resource {

#ifdef CWDEBUG
namespace detail {
using utils::has_print_on::operator<<;

void TextureShaderResourceMember::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_member:" << m_member;
  os << '}';
}

} // namespace detail

void Texture::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_set_key:" << m_descriptor_set_key <<
      ", m_member:" << vk_utils::print_pointer(m_member) <<
      ", m_image_view:&" << m_image_view.get() <<
      ", m_sampler:&" << m_sampler.get();
  os << '}';
}
#endif // CWDEBUG

} // namespace vulkan::shader_builder::shader_resource
