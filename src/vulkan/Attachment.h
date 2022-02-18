#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

struct Attachment
{
  vk::UniqueImage               m_image;
  vk::UniqueImageView           m_image_view;
  vk::UniqueDeviceMemory        m_memory;
};

} // namespace vulkan
