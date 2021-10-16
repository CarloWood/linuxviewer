#pragma once

#include "ImageParameters.h"

namespace vulkan {

struct FrameResourcesData
{
  ImageParameters       m_depth_attachment;
  vk::UniqueFramebuffer m_framebuffer;
  vk::UniqueSemaphore   m_image_available_semaphore;
  vk::UniqueSemaphore   m_finished_rendering_semaphore;
  vk::UniqueFence       m_fence;
};

} // namespace vulkan
