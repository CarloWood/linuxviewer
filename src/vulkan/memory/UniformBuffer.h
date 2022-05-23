#pragma once

#include "Buffer.h"

namespace vulkan::memory {

// These are the defaults for a UniformBuffer.
struct UniformBufferMemoryCreateInfoDefaults
{
  vk::BufferUsageFlags        usage                           = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
  vk::MemoryPropertyFlags     properties                      = vk::MemoryPropertyFlagBits::eHostVisible;
  VmaAllocationCreateFlags    vma_allocation_create_flags     = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                                                VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VmaMemoryUsage              vma_memory_usage                = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo*          allocation_info_out{};
};

struct UniformBuffer : Buffer
{
  using MemoryCreateInfo = UniformBufferMemoryCreateInfoDefaults;

  UniformBuffer(LogicalDevice const* logical_device, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix),
      MemoryCreateInfo memory_create_info = {}) :
    Buffer(logical_device, size,
        { memory_create_info.usage,
          memory_create_info.properties,
          memory_create_info.vma_allocation_create_flags,
          memory_create_info.vma_memory_usage,
          memory_create_info.allocation_info_out }
        COMMA_CWDEBUG_ONLY(ambifix))
    {
      vk::MemoryPropertyFlags memory_property_flags;
      logical_device->get_allocation_memory_properties(m_vh_allocation, &memory_property_flags);
      // See https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html#:~:text=Advanced%20data%20uploading
      // If this flag is not set then a staging buffer is necessary!
      ASSERT((memory_property_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));
    }
};

} // namespace vulkan::memory
