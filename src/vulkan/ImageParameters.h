#pragma once

#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {

// Vulkan Image's parameters container class.
struct ImageParameters
{
  vk::UniqueImage               m_image;
  vk::UniqueImageView           m_image_view;
  vk::UniqueSampler             m_sampler;
  vk::UniqueDeviceMemory        m_memory;
};

// Index for storing ImageParameters as function of Attachment.
using AttachmentIndex = utils::VectorIndex<ImageParameters>;

} // namespace vulkan
