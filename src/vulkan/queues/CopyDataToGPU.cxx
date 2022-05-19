#include "sys.h"
#include "CopyDataToGPU.h"
#include "StagingBufferParameters.h"

namespace task {

CopyDataToGPU::~CopyDataToGPU()
{
  DoutEntering(dc::vulkan, "CopyDataToGPU::~CopyDataToGPU() [" << this << "]");
}

char const* CopyDataToGPU::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(CopyDataToGPU_start);
    AI_CASE_RETURN(CopyDataToGPU_done);
  }
  return direct_base_type::state_str_impl(run_state);
}

void CopyDataToGPU::initialize_impl()
{
  set_state(CopyDataToGPU_start);
}

void CopyDataToGPU::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CopyDataToGPU_start:
    {
      {
        // Create staging buffer and map its memory to copy data from the CPU.
        vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
        m_staging_buffer.m_buffer = vulkan::memory::Buffer(logical_device, m_data.size(), vk::BufferUsageFlagBits::eTransferSrc,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, vk::MemoryPropertyFlagBits::eHostVisible
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_staging_buffer.m_buffer")));
        m_staging_buffer.m_pointer = m_staging_buffer.m_buffer.map_memory();
        // Copy data to the staging buffer, flush and unmap.
        std::memcpy(m_staging_buffer.m_pointer, m_data.data(), m_data.size());
        logical_device->flush_mapped_allocation(m_staging_buffer.m_buffer.m_vh_allocation, 0, VK_WHOLE_SIZE);
        m_staging_buffer.m_buffer.unmap_memory();
      }
      // Set callback to record command buffer to virtual function `record_command_buffer`,
      // the derived class is responsible for appropriate commands to copy the staging buffer to the right destination.
      m_submit_request.set_record_function([this](vulkan::handle::CommandBuffer command_buffer){
        record_command_buffer(command_buffer);
      });
      // Finish the rest of this "immediate submit" by passing control to the base class.
      set_state(ImmediateSubmit_start);
      break;
    }
    case CopyDataToGPU_done:
      finish();
      break;
    default:
      direct_base_type::multiplex_impl(run_state);
      break;
  }
}

} // namespace task
