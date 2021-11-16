#pragma once

#include "Swapchain.h"

namespace vulkan {

struct FrameResourcesData;

// Struct for managing the frame rendering process.
struct CurrentFrameData
{
  FrameResourcesData* m_frame_resources;
  uint32_t m_resource_count;
  uint32_t m_resource_index;
  SwapchainIndex m_swapchain_image_index;
};

} // namespace vulkan
