#include "sys.h"
#include "CopyDataToGPU.h"
#include "StagingBufferParameters.h"
#include "memory/StagingBuffer.h"

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
    AI_CASE_RETURN(CopyDataToGPU_write);
    AI_CASE_RETURN(CopyDataToGPU_flush);
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
      vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
      // Create staging buffer and map its memory to copy data from the CPU.
      m_staging_buffer.m_buffer = vulkan::memory::StagingBuffer(logical_device, m_data_size
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_staging_buffer.m_buffer")));
      m_staging_buffer.m_pointer = m_staging_buffer.m_buffer.map_memory();
      set_state(CopyDataToGPU_write);
    }
    [[fallthrough]];
    case CopyDataToGPU_write:
    {
      // Copy data to the staging buffer.
      unsigned char* dst = static_cast<unsigned char*>(m_staging_buffer.m_pointer);
      uint32_t const fragment_size = m_data_feeder->fragment_size();
      int const fragment_count = m_data_feeder->fragment_count();
      int fragments;
      for (int total_fragments = 0; total_fragments < fragment_count; total_fragments += fragments)
      {
        fragments = m_data_feeder->next_batch();
        m_data_feeder->get_fragments(dst);
        dst += fragments * fragment_size;
      }
      set_state(CopyDataToGPU_flush);
    }
    [[fallthrough]];
    case CopyDataToGPU_flush:
    {
      vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
      // Once everything is written to the staging buffer, flush and unmap it.
      logical_device->flush_mapped_allocation(m_staging_buffer.m_buffer.m_vh_allocation, 0, VK_WHOLE_SIZE);
      m_staging_buffer.m_buffer.unmap_memory();
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
