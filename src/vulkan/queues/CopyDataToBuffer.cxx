#include "sys.h"
#include "CopyDataToBuffer.h"
#include "StagingBufferParameters.h"

namespace task {

CopyDataToBuffer::~CopyDataToBuffer()
{
  DoutEntering(dc::vulkan, "CopyDataToBuffer::~CopyDataToBuffer() [" << this << "]");
}

char const* CopyDataToBuffer::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(CopyDataToBuffer_start);
    AI_CASE_RETURN(CopyDataToBuffer_done);
  }
  return direct_base_type::state_str_impl(run_state);
}

void CopyDataToBuffer::record_command_buffer(vulkan::ImmediateSubmitRequest::command_buffer_wat command_buffer_w)
{
  DoutEntering(dc::vulkan, "CopyDataToBuffer::record_command_buffer(" << command_buffer_w << ")");

  command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

  vk::BufferMemoryBarrier pre_transfer_buffer_memory_barrier{
    .srcAccessMask = m_current_buffer_access,
    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = m_vh_target_buffer,
    .offset = m_buffer_offset,
    .size = m_data.size()
  };
  command_buffer_w->pipelineBarrier(m_generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(0), {}, { pre_transfer_buffer_memory_barrier }, {});

  vk::BufferCopy buffer_copy_region{
    .srcOffset = 0,
    .dstOffset = m_buffer_offset,
    .size = m_data.size()
  };
  command_buffer_w->copyBuffer(*m_staging_buffer.m_buffer.m_buffer, m_vh_target_buffer, { buffer_copy_region });

  vk::BufferMemoryBarrier post_transfer_buffer_memory_barrier{
    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
    .dstAccessMask = m_new_buffer_access,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = m_vh_target_buffer,
    .offset = m_buffer_offset,
    .size = m_data.size()
  };
  command_buffer_w->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, m_consuming_stages, vk::DependencyFlags(0), {}, { post_transfer_buffer_memory_barrier }, {});
  command_buffer_w->end();
}

void CopyDataToBuffer::initialize_impl()
{
  set_state(CopyDataToBuffer_start);
}

void CopyDataToBuffer::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CopyDataToBuffer_start:
    {
      // Create staging buffer and map its memory to copy data from the CPU.
      {
        vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
        m_staging_buffer.m_buffer = logical_device->create_buffer(m_data.size(), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_staging_buffer.m_buffer")));
        m_staging_buffer.m_pointer = logical_device->map_memory(*m_staging_buffer.m_buffer.m_memory, 0, m_data.size());

        std::memcpy(m_staging_buffer.m_pointer, m_data.data(), m_data.size());

        vk::MappedMemoryRange memory_range{
          .memory = *m_staging_buffer.m_buffer.m_memory,
          .offset = 0,
          .size = VK_WHOLE_SIZE
        };
        logical_device->flush_mapped_memory_ranges({ memory_range });
        logical_device->unmap_memory(*m_staging_buffer.m_buffer.m_memory);
      }
      m_submit_request.set_record_function([this](vulkan::ImmediateSubmitRequest::command_buffer_wat command_buffer_w){
        record_command_buffer(command_buffer_w);
      });
      set_state(ImmediateSubmit_start);
      break;
    }
    case CopyDataToBuffer_done:
      finish();
      break;
    default:
      direct_base_type::multiplex_impl(run_state);
      break;
  }
}

} // namespace task
