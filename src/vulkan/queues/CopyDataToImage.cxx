#include "sys.h"
#include "CopyDataToImage.h"
#include "StagingBufferParameters.h"

namespace task {

CopyDataToImage::~CopyDataToImage()
{
  DoutEntering(dc::vulkan, "CopyDataToImage::~CopyDataToImage() [" << this << "]");
}

char const* CopyDataToImage::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(CopyDataToImage_start);
    AI_CASE_RETURN(CopyDataToImage_done);
  }
  return direct_base_type::state_str_impl(run_state);
}

void CopyDataToImage::record_command_buffer(vulkan::handle::CommandBuffer command_buffer)
{
  DoutEntering(dc::vulkan, "CopyDataToImage::record_command_buffer(" << command_buffer << ")");

  command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

  vk::ImageMemoryBarrier pre_transfer_image_memory_barrier{
    .srcAccessMask = m_current_image_access,
    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
    .oldLayout = m_current_image_layout,
    .newLayout = vk::ImageLayout::eTransferDstOptimal,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = m_vh_target_image,
    .subresourceRange = m_image_subresource_range
  };
  command_buffer->pipelineBarrier(m_generating_stages, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(0), {}, {}, { pre_transfer_image_memory_barrier });

  std::vector<vk::BufferImageCopy> buffer_image_copy;
  buffer_image_copy.reserve(m_image_subresource_range.levelCount);
  for (uint32_t i = m_image_subresource_range.baseMipLevel; i < m_image_subresource_range.baseMipLevel + m_image_subresource_range.levelCount; ++i)
  {
    buffer_image_copy.emplace_back(vk::BufferImageCopy{
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = vk::ImageSubresourceLayers{
        .aspectMask = m_image_subresource_range.aspectMask,
        .mipLevel = i,
        .baseArrayLayer = m_image_subresource_range.baseArrayLayer,
        .layerCount = m_image_subresource_range.layerCount
      },
      .imageOffset = vk::Offset3D{},
      .imageExtent = vk::Extent3D{
        .width = m_extent.width,
        .height = m_extent.height,
        .depth = 1
      }
    });
  }
  command_buffer->copyBufferToImage(m_staging_buffer.m_buffer.m_vh_buffer, m_vh_target_image, vk::ImageLayout::eTransferDstOptimal, buffer_image_copy);

  vk::ImageMemoryBarrier post_transfer_image_memory_barrier{
    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
    .dstAccessMask = m_new_image_access,
    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
    .newLayout = m_new_image_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = m_vh_target_image,
    .subresourceRange = m_image_subresource_range
  };
  command_buffer->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, m_consuming_stages, vk::DependencyFlags(0), {}, {}, { post_transfer_image_memory_barrier });
  command_buffer->end();
}

void CopyDataToImage::initialize_impl()
{
  set_state(CopyDataToImage_start);
}

void CopyDataToImage::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case CopyDataToImage_start:
    {
      // Create staging buffer and map its memory to copy data from the CPU.
      {
        vulkan::LogicalDevice const* logical_device = m_submit_request.logical_device();
        m_staging_buffer.m_buffer = vulkan::memory::Buffer(logical_device, m_data.size(), vk::BufferUsageFlagBits::eTransferSrc,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, vk::MemoryPropertyFlagBits::eHostVisible
            COMMA_CWDEBUG_ONLY(debug_name_prefix("m_staging_buffer.m_buffer")));
        m_staging_buffer.m_pointer = m_staging_buffer.m_buffer.map_memory();
        std::memcpy(m_staging_buffer.m_pointer, m_data.data(), m_data.size());
        logical_device->flush_mapped_allocation(m_staging_buffer.m_buffer.m_vh_allocation, 0, VK_WHOLE_SIZE);
        m_staging_buffer.m_buffer.unmap_memory();
      }
      m_submit_request.set_record_function([this](vulkan::handle::CommandBuffer command_buffer){
        record_command_buffer(command_buffer);
      });
      set_state(ImmediateSubmit_start);
      break;
    }
    case CopyDataToImage_done:
      finish();
      break;
    default:
      direct_base_type::multiplex_impl(run_state);
      break;
  }
}

} // namespace task
