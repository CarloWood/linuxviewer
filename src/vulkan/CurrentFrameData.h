#pragma once

#include "Swapchain.h"
#include "FrameResourceIndex.h" // Also forward declares FrameResourcesData.

namespace vulkan {

// Struct for managing the frame rendering process.
struct CurrentFrameData
{
  FrameResourcesData* m_frame_resources;
  FrameResourceIndex m_resource_count;
  FrameResourceIndex m_resource_index;
};

} // namespace vulkan
