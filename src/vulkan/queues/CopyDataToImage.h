#pragma once

#include "ImmediateSubmit.h"
#include "StagingBufferParameters.h"
#include "vk_utils/print_flags.h"
#include <vector>

namespace task {

class CopyDataToImage final : public ImmediateSubmit
{
 private:
  std::vector<std::byte> m_data;        // FIXME: I don't think this buffer should exist. We should write more directly into whatever it is that we're writing to.
  vk::Image m_vh_target_image;
  vk::Extent2D m_extent;
  vk_defaults::ImageSubresourceRange const m_image_subresource_range;
  vk::ImageLayout m_current_image_layout;
  vk::AccessFlags m_current_image_access;
  vk::PipelineStageFlags m_generating_stages;
  vk::ImageLayout m_new_image_layout;
  vk::AccessFlags m_new_image_access;
  vk::PipelineStageFlags m_consuming_stages;
  vulkan::StagingBufferParameters m_staging_buffer;

 protected:
  using direct_base_type = ImmediateSubmit;

  // The different states of this task.
  enum CopyDataToImage_state_type {
    CopyDataToImage_start = direct_base_type::state_end,
    CopyDataToImage_done
  };

 public:
  static constexpr state_type state_end = CopyDataToImage_done + 1;

  // Construct a CopyDataToImage object.
  CopyDataToImage(vulkan::LogicalDevice const* logical_device,
      uint32_t data_size, vk::Image vh_target_image, vk::Extent2D extent, vk_defaults::ImageSubresourceRange image_subresource_range,
      vk::ImageLayout current_image_layout, vk::AccessFlags current_image_access, vk::PipelineStageFlags generating_stages,
      vk::ImageLayout new_image_layout, vk::AccessFlags new_image_access, vk::PipelineStageFlags consuming_stages
      COMMA_CWDEBUG_ONLY(bool debug)) :
    ImmediateSubmit({logical_device, this}, CopyDataToImage_done COMMA_CWDEBUG_ONLY(debug)),
    m_data(data_size), m_vh_target_image(vh_target_image), m_extent(extent), m_image_subresource_range(image_subresource_range),
    m_current_image_layout(current_image_layout), m_current_image_access(current_image_access), m_generating_stages(generating_stages),
    m_new_image_layout(new_image_layout), m_new_image_access(new_image_access), m_consuming_stages(consuming_stages)
  {
    DoutEntering(dc::vulkan, "CopyDataToImage(" << logical_device << ", " << data_size << ", " << vh_target_image << ")");
  }

  // FIXME: this should not be here.
  std::vector<std::byte>& get_buf() { return m_data; }

 private:
  void record_command_buffer(vulkan::handle::CommandBuffer command_buffer);

 protected:
  ~CopyDataToImage() override;

  void initialize_impl() override;
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
