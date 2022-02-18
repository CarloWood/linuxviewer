#pragma once

#include "Attachment.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include "utils/Vector.h"
#include <memory>

namespace vulkan {

struct FrameResourcesData
{
  utils::Vector<Attachment, rendergraph::AttachmentIndex> m_attachments;

  // Too specialized?
  static constexpr vk::CommandPoolCreateFlags::MaskType pool_type = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  using command_pool_type = CommandPool<pool_type>;
  command_pool_type       m_command_pool;

  // Command buffers (currently only one).
  handle::CommandBuffer   m_command_buffer;                     // Freed when the command pool is destructed.

  // Fence that signals when all (aka, the last) command buffers have finished.
  vk::UniqueFence         m_command_buffers_completed;          // This fence should be signaled when the last command buffer used for this frame completed.

  FrameResourcesData(
      size_t number_of_attachments,
      // Arguments for m_command_pool.
      LogicalDevice const* logical_device,
      QueueFamilyPropertiesIndex queue_family
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& command_pool_debug_name)) :
    m_attachments(number_of_attachments),
    m_command_pool(logical_device, queue_family COMMA_CWDEBUG_ONLY(command_pool_debug_name)) { }

  ~FrameResourcesData()
  {
    Dout(dc::vulkan, "Destroying m_command_buffers_completed " << *m_command_buffers_completed);
  }
};

} // namespace vulkan
