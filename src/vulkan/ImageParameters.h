#pragma once

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

} // namespace vulkan
