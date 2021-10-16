#pragma once

namespace vulkan {

struct FrameResourcesData;

// Struct for managing the frame rendering process.
struct CurrentFrameData
{
  FrameResourcesData* m_frame_resources;
  uint32_t            m_resource_count;
  uint32_t            m_resource_index;
  uint32_t            m_swapchain_image_index;
};

} // namespace vulkan
