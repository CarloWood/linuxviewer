#pragma once

#include "ImageParameters.h"

namespace vulkan {

struct FrameResourcesData
{
  ImageParameters         m_depth_attachment;
  vk::UniqueFramebuffer   m_framebuffer;
  vk::UniqueSemaphore     m_image_available_semaphore;
  vk::UniqueSemaphore     m_finished_rendering_semaphore;
  vk::UniqueFence         m_fence;
  // Too specialized?
  vk::UniqueCommandPool   m_command_pool;
  vk::UniqueCommandBuffer m_pre_command_buffer;
  vk::UniqueCommandBuffer m_post_command_buffer;
};

} // namespace vulkan
