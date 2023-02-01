#pragma once

#include "CopyDataToGPU.h"
#include "vk_utils/print_flags.h"

namespace vulkan::task {

class CopyDataToImage final : public CopyDataToGPU
{
 private:
  vk::Image m_vh_target_image;
  vk::Extent2D m_extent;
  vk_defaults::ImageSubresourceRange const m_image_subresource_range;
  vk::ImageLayout m_current_image_layout;
  vk::AccessFlags m_current_image_access;
  vk::PipelineStageFlags m_generating_stages;
  vk::ImageLayout m_new_image_layout;
  vk::AccessFlags m_new_image_access;
  vk::PipelineStageFlags m_consuming_stages;

 public:
  // Construct a CopyDataToImage object.
  CopyDataToImage(vulkan::LogicalDevice const* logical_device,
      uint32_t data_size, vk::Image vh_target_image, vk::Extent2D extent, vk_defaults::ImageSubresourceRange image_subresource_range,
      vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
      vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages
      COMMA_CWDEBUG_ONLY(bool debug)) :
    CopyDataToGPU(logical_device, data_size COMMA_CWDEBUG_ONLY(debug)),
    m_vh_target_image(vh_target_image), m_extent(extent), m_image_subresource_range(image_subresource_range),
    m_current_image_layout(current_image_layout), m_current_image_access(current_image_access), m_generating_stages(generating_stages),
    m_new_image_layout(new_image_layout), m_new_image_access(new_image_access), m_consuming_stages(consuming_stages)
  {
    DoutEntering(dc::vulkan(mSMDebug), "CopyDataToImage(" << logical_device << ", " << data_size << ", " << vh_target_image << ", " <<
        extent << ", " << image_subresource_range << ", " << current_image_layout << ", " << current_image_access << ", " <<
        generating_stages << ", " << new_image_layout << ", " << new_image_access << ", " << consuming_stages << ")");
  }

 private:
  void record_command_buffer(handle::CommandBuffer command_buffer) override;
};

} // namespace vulkan::task
