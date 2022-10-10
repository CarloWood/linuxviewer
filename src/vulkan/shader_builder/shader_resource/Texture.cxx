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

void Texture::update_descriptor_set(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, bool has_frame_resource) const
{
  DoutEntering(dc::shaderresource, "Texture::update_descriptor_set(" << owning_window << ", " << descriptor_set << ", " << binding << ", " << std::boolalpha << has_frame_resource << ")");
  // Update vh_descriptor_set binding `binding` with this texture.
  std::vector<vk::DescriptorImageInfo> image_infos = {
    {
      .sampler = *m_sampler,
      .imageView = *m_image_view,
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    }
  };
  if (has_frame_resource)
    for (FrameResourceIndex frame_index{0}; frame_index < owning_window->max_number_of_frame_resources(); ++frame_index)
      owning_window->logical_device()->update_descriptor_sets(descriptor_set[frame_index], vk::DescriptorType::eCombinedImageSampler, binding, 0 /*array_element*/, image_infos);
  else
    owning_window->logical_device()->update_descriptor_sets(descriptor_set, vk::DescriptorType::eCombinedImageSampler, binding, 0 /*array_element*/, image_infos);
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
