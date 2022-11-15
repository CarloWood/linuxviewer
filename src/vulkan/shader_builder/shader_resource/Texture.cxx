#include "sys.h"
#include "Texture.h"
#include "SynchronousWindow.h"
#include "queues/CopyDataToImage.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#include "utils/has_print_on.h"
#endif

namespace vulkan::shader_builder::shader_resource {

void Texture::instantiate(task::SynchronousWindow const* CWDEBUG_ONLY(owning_window)
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  DoutEntering(dc::shaderresource, "Texture::instantiate(" << owning_window << ", \"" << ambifix.object_name() << "\")");
}

void Texture::upload(vk::Extent2D extent, vulkan::ImageViewKind const& image_view_kind,
    task::SynchronousWindow const* resource_owner,      // The window that determines the life-time of this texture.
    std::unique_ptr<vulkan::DataFeeder> texture_data_feeder,
    AIStatefulTask* parent, AIStatefulTask::condition_type texture_ready)
{
  size_t const data_size = extent.width * extent.height * vk_utils::format_component_count(image_view_kind.image_kind()->format);

  auto copy_data_to_image = statefultask::create<task::CopyDataToImage>(m_logical_device, data_size,
            m_vh_image, extent, vk_defaults::ImageSubresourceRange{},
            vk::ImageLayout::eUndefined, vk::AccessFlags(0), vk::PipelineStageFlagBits::eTopOfPipe,
            vk::ImageLayout::eShaderReadOnlyOptimal, vk::AccessFlagBits::eShaderRead, vk::PipelineStageFlagBits::eFragmentShader
            COMMA_CWDEBUG_ONLY(true));

  copy_data_to_image->set_resource_owner(resource_owner);       // Wait for this task to finish before destroying the owning window, because the window owns this texture.
  copy_data_to_image->set_data_feeder(std::move(texture_data_feeder));
  copy_data_to_image->run(vulkan::Application::instance().low_priority_queue(), parent, texture_ready, AIStatefulTask::signal_parent);
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

void Texture::prepare_shader_resource_declaration(descriptor::SetIndexHint set_index_hint, pipeline::ShaderInputData* shader_input_data) const
{
  shader_input_data->prepare_texture_declaration(*this, set_index_hint);
}

} // namespace vulkan::shader_builder::shader_resource

#ifdef CWDEBUG
namespace vulkan::descriptor {
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
  os << ", m_member:" << vk_utils::print_pointer(m_member);
  os << '}';
}
#endif // CWDEBUG

} // namespace vulkan::descriptor
