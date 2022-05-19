#pragma once

#include "CopyDataToGPU.h"
#include "vk_utils/print_flags.h"

namespace task {

class CopyDataToBuffer final : public CopyDataToGPU
{
 private:
  vk::Buffer m_vh_target_buffer;
  vk::DeviceSize m_buffer_offset;
  vk::AccessFlags m_current_buffer_access;
  vk::PipelineStageFlags m_generating_stages;
  vk::AccessFlags m_new_buffer_access;
  vk::PipelineStageFlags m_consuming_stages;

 public:
  // Construct a CopyDataToBuffer object.
  CopyDataToBuffer(vulkan::LogicalDevice const* logical_device,
      uint32_t data_size, vk::Buffer vh_target_buffer,
      vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
      vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages
      COMMA_CWDEBUG_ONLY(bool debug)) :
    CopyDataToGPU(logical_device, data_size COMMA_CWDEBUG_ONLY(debug)),
    m_vh_target_buffer(vh_target_buffer), m_buffer_offset(buffer_offset),
    m_current_buffer_access(current_buffer_access), m_generating_stages(generating_stages),
    m_new_buffer_access(new_buffer_access), m_consuming_stages(consuming_stages)
  {
    DoutEntering(dc::vulkan, "CopyDataToBuffer(" << logical_device << ", " << data_size << ", " << vh_target_buffer <<
        ", " << buffer_offset << ", " << current_buffer_access << ", " << generating_stages <<
        ", " << new_buffer_access << ", " << consuming_stages << ")");
  }

 private:
  void record_command_buffer(vulkan::handle::CommandBuffer command_buffer) override;
};

} // namespace task
