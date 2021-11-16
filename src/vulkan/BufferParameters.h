#pragma once

#include <vulkan/vulkan.hpp>
#include <cstdint>

namespace vulkan {

// Vulkan Buffer's parameters container class.
struct BufferParameters
{
  vk::UniqueBuffer m_buffer;
  vk::UniqueDeviceMemory m_memory;
  uint32_t m_size = {};
};

} // namespace vulkan
