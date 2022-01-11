#pragma once

#include "Swapchain.h"
#include "utils/Vector.h"

namespace vulkan {

struct FrameResourcesData;
using FrameResourceIndex = utils::VectorIndex<FrameResourcesData>;

// Struct for managing the frame rendering process.
struct CurrentFrameData
{
  FrameResourcesData* m_frame_resources;
  FrameResourceIndex m_resource_count;
  FrameResourceIndex m_resource_index;
};

} // namespace vulkan
