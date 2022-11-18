#include "sys.h"
#include "Texture.h"
#include "SynchronousWindow.h"
#include "queues/CopyDataToImage.h"

namespace vulkan::shader_builder::shader_resource {

void Texture::upload(vk::Extent2D extent, vulkan::ImageViewKind const& image_view_kind,
    task::SynchronousWindow const* resource_owner,      // The window that determines the life-time of this texture.
    std::unique_ptr<vulkan::DataFeeder> texture_data_feeder,
    AIStatefulTask* parent, AIStatefulTask::condition_type texture_ready)
{
  DoutEntering(dc::vulkan, "Texture::upload(" << extent << ", " << image_view_kind << ", " << resource_owner << ", " << texture_data_feeder << ", " << parent << ", " << texture_ready << ")");

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

void Texture::update_descriptor_set_old(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding) const
{
  DoutEntering(dc::shaderresource, "Texture::update_descriptor_set_old(" << owning_window << ", " << descriptor_set << ", " << binding << ")");
  // Update vh_descriptor_set binding `binding` with this texture.
  std::vector<vk::DescriptorImageInfo> image_infos = {
    {
      .sampler = *m_sampler,
      .imageView = *m_image_view,
      .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    }
  };
  owning_window->logical_device()->update_descriptor_sets(descriptor_set, vk::DescriptorType::eCombinedImageSampler, binding, 0 /*array_element*/, image_infos);
}

#ifdef CWDEBUG
void Texture::print_on(std::ostream& os) const
{
  os << '{';
  memory::Image::print_on(os);
  os << "m_image_view:" << m_image_view <<
      ", m_sampler:" << m_sampler;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder::shader_resource
