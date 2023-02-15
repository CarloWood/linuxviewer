#include "sys.h"
#include "CopyDataToGPU.h"
#include "SynchronousWindow.h"
#include "memory/StagingBuffer.h"

namespace vulkan::task {

CopyDataToGPU::~CopyDataToGPU()
{
  DoutEntering(dc::statefultask(mSMDebug), "~CopyDataToGPU() [" << this << "]");
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
  DoutEntering(dc::statefultask(mSMDebug), "CopyDataToGPU::initialize_impl() [" << this << "]");
  set_state(CopyDataToGPU_start);
  if (m_resource_owner)
  {
    try
    {
      // m_resource_owner is "threadsafe-const" - but increment() is just thread-safe, not const.
      // Therefore we have to cast away the const.
      const_cast<SynchronousWindow*>(m_resource_owner)->m_task_counter_gate.increment();
    }
    catch (std::exception const&)
    {
      m_resource_owner = nullptr;       // Stop finish_impl from calling decrement().
      abort();
    }
  }
}

void CopyDataToGPU::finish_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "CopyDataToGPU::finish_impl() [" << this << "]");
  if (m_resource_owner)
    // See above.
    const_cast<SynchronousWindow*>(m_resource_owner)->m_task_counter_gate.decrement();
}

void CopyDataToGPU::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CopyDataToGPU_start:
    {
      ZoneScopedN("CopyDataToGPU_start");
      vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
      // Create staging buffer and map its memory to copy data from the CPU.
      m_staging_buffer = memory::StagingBuffer(logical_device, m_data_size
          COMMA_CWDEBUG_ONLY(debug_name_prefix("m_staging_buffer")));
      set_state(CopyDataToGPU_write);
    }
    Dout(dc::statefultask(mSMDebug), "Falling through to CopyDataToGPU_write.");
    [[fallthrough]];
    case CopyDataToGPU_write:
    {
      ZoneScopedN("CopyDataToGPU_write");
      // Copy data to the staging buffer.
      unsigned char* dst = static_cast<unsigned char*>(m_staging_buffer.m_pointer);
      uint32_t const chunk_size = m_data_feeder->chunk_size();
      int const chunk_count = m_data_feeder->chunk_count();
      int chunks;
      for (int total_chunks = 0; total_chunks < chunk_count; total_chunks += chunks)
      {
        chunks = m_data_feeder->next_batch();
        m_data_feeder->get_chunks(dst);
        dst += chunks * chunk_size;
      }
      set_state(CopyDataToGPU_flush);
    }
    Dout(dc::statefultask(mSMDebug), "Falling through to CopyDataToGPU_flush.");
    [[fallthrough]];
    case CopyDataToGPU_flush:
    {
      ZoneScopedN("CopyDataToGPU_flush");
      vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
      // Once everything is written to the staging buffer and flush.
      logical_device->flush_mapped_allocation(m_staging_buffer.m_vh_allocation, 0, VK_WHOLE_SIZE);
      // Set callback to record command buffer to virtual function `record_command_buffer`,
      // the derived class is responsible for appropriate commands to copy the staging buffer to the right destination.
      m_submit_request.set_record_function([this](handle::CommandBuffer command_buffer){
        record_command_buffer(command_buffer);
      });
      // Finish the rest of this "immediate submit" by passing control to the base class.
      run_state = ImmediateSubmit_start;
      break;
    }
    case CopyDataToGPU_done:
    {
      ZoneScopedN("CopyDataToGPU_done");
      finish();
      return;
    }
  }
  direct_base_type::multiplex_impl(run_state);
}

} // namespace vulkan::task
