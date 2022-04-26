#pragma once

#include "ImmediateSubmit.h"
#include "StagingBufferParameters.h"
#include "vk_utils/print_flags.h"
#include <vector>

namespace task {

class CopyDataToBuffer final : public ImmediateSubmit
{
 private:
//  uint32_t m_data_size;
  std::vector<std::byte> m_data;                        // FIXME: I don't think this buffer should exist. We should write more directly into whatever it is that we're writing to.
  vk::Buffer m_target_buffer;
  vk::DeviceSize m_buffer_offset;
  vk::AccessFlags m_current_buffer_access;
  vk::PipelineStageFlags m_generating_stages;
  vk::AccessFlags m_new_buffer_access;
  vk::PipelineStageFlags m_consuming_stages;
  vulkan::StagingBufferParameters m_staging_buffer;

 protected:
  using direct_base_type = ImmediateSubmit;

  // The different states of this task.
  enum CopyDataToBuffer_state_type {
    CopyDataToBuffer_start = direct_base_type::state_end,
    CopyDataToBuffer_done
  };

 public:
  static constexpr state_type state_end = CopyDataToBuffer_done + 1;

  // Construct a CopyDataToBuffer object.
  CopyDataToBuffer(vulkan::LogicalDevice const* logical_device,
      uint32_t data_size, /*void const* data,*/ vk::Buffer target_buffer,
      vk::DeviceSize buffer_offset, vk::AccessFlags current_buffer_access, vk::PipelineStageFlags generating_stages,
      vk::AccessFlags new_buffer_access, vk::PipelineStageFlags consuming_stages
      COMMA_CWDEBUG_ONLY(bool debug)) :
    ImmediateSubmit({logical_device, this}, CopyDataToBuffer_done COMMA_CWDEBUG_ONLY(debug)),
    /*m_data_size(data_size),*/ m_data(data_size), m_target_buffer(target_buffer),
    m_buffer_offset(buffer_offset), m_current_buffer_access(current_buffer_access), m_generating_stages(generating_stages),
    m_new_buffer_access(new_buffer_access), m_consuming_stages(consuming_stages)
  {
    DoutEntering(dc::vulkan, "CopyDataToBuffer(" << logical_device << ", " << data_size << ", " << /*data << ", " <<*/ target_buffer <<
        ", " << buffer_offset << ", " << current_buffer_access << ", " << generating_stages <<
        ", " << new_buffer_access << ", " << consuming_stages << ")");
  }

  // FIXME: this should not be here.
  std::vector<std::byte>& get_buf() { return m_data; }

 private:
  void record_command_buffer(vulkan::ImmediateSubmitRequest::command_buffer_wat command_buffer_w);

 protected:
  ~CopyDataToBuffer() override;

  void initialize_impl() override;
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
