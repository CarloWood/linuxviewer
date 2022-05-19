#pragma once

#include "ImmediateSubmit.h"
#include "StagingBufferParameters.h"
#include <vector>

namespace task {

class CopyDataToGPU : public ImmediateSubmit
{
 protected:
  std::vector<std::byte> m_data;        // FIXME: I don't think this buffer should exist. We should write more directly into whatever it is that we're writing to.
  vulkan::StagingBufferParameters m_staging_buffer;

 protected:
  using direct_base_type = ImmediateSubmit;

  // The different states of this task.
  enum CopyDataToGPU_state_type {
    CopyDataToGPU_start = direct_base_type::state_end,
    CopyDataToGPU_done
  };

 public:
  static constexpr state_type state_end = CopyDataToGPU_done + 1;

  // Construct a CopyDataToGPU object.
  CopyDataToGPU(vulkan::LogicalDevice const* logical_device, uint32_t data_size
      COMMA_CWDEBUG_ONLY(bool debug)) :
    ImmediateSubmit({logical_device, this}, CopyDataToGPU_done COMMA_CWDEBUG_ONLY(debug)),
    m_data(data_size)
  {
    DoutEntering(dc::vulkan, "CopyDataToGPU(" << logical_device << ", " << data_size << ")");
  }

  // FIXME: this should not be here.
  std::vector<std::byte>& get_buf() { return m_data; }

 private:
  virtual void record_command_buffer(vulkan::handle::CommandBuffer command_buffer) = 0;

 protected:
  ~CopyDataToGPU() override;

  void initialize_impl() override;
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
