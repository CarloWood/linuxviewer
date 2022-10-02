#include "sys.h"
#include "Texture.h"
#include "SynchronousWindow.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#include "utils/has_print_on.h"
#endif

namespace vulkan::shader_builder::shader_resource {

void Texture::create(task::SynchronousWindow const* owning_window)
{
#ifdef CWDEBUG
  add_ambifix(owning_window->debug_name_prefix("PipelineFactory::"));
#endif
}

void Texture::update_descriptor_set(task::SynchronousWindow const* owning_window, vk::DescriptorSet vh_descriptor_set, uint32_t binding) const
{
  // Update descriptor set of m_sample_texture.
  std::vector<vk::DescriptorImageInfo> image_infos = {
    {
      .sampler = *m_sampler,
      .imageView = *m_image_view,
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    }
  };
  // The 'top' descriptor set also contains the texture.
  owning_window->logical_device()->update_descriptor_set(vh_descriptor_set, vk::DescriptorType::eCombinedImageSampler, binding, 0, image_infos);
  //create_uniform_buffers
}

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
  os << "(Base)";
  Base::print_on(os);
  os << ", m_member:" << vk_utils::print_pointer(m_member) <<
        ", m_image_view:&" << m_image_view.get() <<
        ", m_sampler:&" << m_sampler.get();
  os << '}';
}
#endif // CWDEBUG

} // namespace vulkan::shader_builder::shader_resource
