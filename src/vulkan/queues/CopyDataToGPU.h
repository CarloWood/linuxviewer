#pragma once

#include "ImmediateSubmit.h"
#include "StagingBufferParameters.h"
#include "memory/DataFeeder.h"
#include <vector>

namespace task {

class CopyDataToGPU : public ImmediateSubmit
{
 protected:
  std::unique_ptr<vulkan::DataFeeder> m_data_feeder;
  vulkan::StagingBufferParameters m_staging_buffer;
  uint32_t m_data_size;

 protected:
  using direct_base_type = ImmediateSubmit;

  // The different states of this task.
  enum CopyDataToGPU_state_type {
    CopyDataToGPU_start = direct_base_type::state_end,
    CopyDataToGPU_write,
    CopyDataToGPU_flush,
    CopyDataToGPU_done
  };

 public:
  static constexpr state_type state_end = CopyDataToGPU_done + 1;

  // Construct a CopyDataToGPU object.
  CopyDataToGPU(vulkan::LogicalDevice const* logical_device, uint32_t data_size
      COMMA_CWDEBUG_ONLY(bool debug)) :
    ImmediateSubmit({logical_device, this}, CopyDataToGPU_done COMMA_CWDEBUG_ONLY(debug)),
    m_data_size(data_size)
  {
    DoutEntering(dc::vulkan, "CopyDataToGPU(" << logical_device << ", " << data_size << ")");
  }

  void set_data_feeder(std::unique_ptr<vulkan::DataFeeder> data_feeder)
  {
    m_data_feeder = std::move(data_feeder);
  }

 private:
  virtual void record_command_buffer(vulkan::handle::CommandBuffer command_buffer) = 0;

 protected:
  ~CopyDataToGPU() override;

  void initialize_impl() override;
  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
