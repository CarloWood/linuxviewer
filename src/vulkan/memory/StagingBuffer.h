#pragma once

#include "Buffer.h"

namespace vulkan::memory {

// These are the defaults for a StagingBuffer.
struct StagingBufferMemoryCreateInfoDefaults
{
  vk::BufferUsageFlags        usage                           = vk::BufferUsageFlagBits::eTransferSrc;
  vk::MemoryPropertyFlags     properties                      = vk::MemoryPropertyFlagBits::eHostVisible;
  VmaAllocationCreateFlags    vma_allocation_create_flags     = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  VmaMemoryUsage              vma_memory_usage                = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo*          allocation_info_out{};
};

struct StagingBuffer : Buffer
{
  using MemoryCreateInfo = StagingBufferMemoryCreateInfoDefaults;

  // This whole constructor is optimized away (with -O1) but has different defaults for memory_create_info.
  StagingBuffer(LogicalDevice const* logical_device, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix),
      MemoryCreateInfo memory_create_info = {}) :
    Buffer(logical_device, size,
        { memory_create_info.usage,
          memory_create_info.properties,
          memory_create_info.vma_allocation_create_flags,
          memory_create_info.vma_memory_usage,
          memory_create_info.allocation_info_out }
        COMMA_CWDEBUG_ONLY(ambifix)) { }
};

} // namespace vulkan::memory
