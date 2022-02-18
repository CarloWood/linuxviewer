#pragma once

#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {

// Data collection used for textures.
struct Texture
{
  vk::UniqueImage               m_image;
  vk::UniqueImageView           m_image_view;
  vk::UniqueSampler             m_sampler;
  vk::UniqueDeviceMemory        m_memory;
};

} // namespace vulkan
