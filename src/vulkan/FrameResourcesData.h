#pragma once

#include "ImageParameters.h"
#include "CommandPool.h"
#include "CommandBuffer.h"
#include <memory>

namespace vulkan {

struct FrameResourcesData
{
  ImageParameters         m_depth_attachment;

  // Too specialized?
  using command_pool_type = CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT>;
  command_pool_type       m_command_pool;
  handle::CommandBuffer   m_pre_command_buffer;                 // Freed when the command pool is destructed.
  vk::UniqueFence         m_command_buffers_completed;          // This fence should be signaled when the last command buffer used for this frame completed.
//  handle::CommandBuffer   m_post_command_buffer;              // Idem.

  FrameResourcesData(
      // Arguments for m_command_pool.
      LogicalDevice const* logical_device,
      QueueFamilyPropertiesIndex queue_family
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& command_pool_debug_name)) :
    m_command_pool(logical_device, queue_family COMMA_CWDEBUG_ONLY(command_pool_debug_name)) { }
};

} // namespace vulkan
