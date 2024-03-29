#include "sys.h"
#include "CopyDataToBuffer.h"

namespace vulkan::task {

void CopyDataToBuffer::record_command_buffer(handle::CommandBuffer command_buffer)
{
  DoutEntering(dc::vulkan(mSMDebug), "CopyDataToBuffer::record_command_buffer(" << command_buffer << ") [" << this << "]");

  command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

  vk::BufferMemoryBarrier pre_transfer_buffer_memory_barrier{
    .srcAccessMask = m_current_buffer_access,
    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = m_vh_target_buffer,
    .offset = m_buffer_offset,
    .size = m_data_size
  };
  command_buffer.pipelineBarrier(m_generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(0), {}, { pre_transfer_buffer_memory_barrier }, {});

  vk::BufferCopy buffer_copy_region{
    .srcOffset = 0,
    .dstOffset = m_buffer_offset,
    .size = m_data_size
  };
  Dout(dc::always, "Preparing to transfer from " << m_staging_buffer.m_vh_buffer << " to " << m_vh_target_buffer << ": " << buffer_copy_region);
  command_buffer.copyBuffer(m_staging_buffer.m_vh_buffer, m_vh_target_buffer, { buffer_copy_region });

  vk::BufferMemoryBarrier post_transfer_buffer_memory_barrier{
    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
    .dstAccessMask = m_new_buffer_access,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .buffer = m_vh_target_buffer,
    .offset = m_buffer_offset,
    .size = m_data_size
  };
  command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, m_consuming_stages, vk::DependencyFlags(0), {}, { post_transfer_buffer_memory_barrier }, {});
  command_buffer.end();
}

} // namespace vulkan::task
