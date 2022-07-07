#pragma once

#include "Buffer.h"

namespace vulkan::memory {

// These are the defaults for a StagingBuffer when using vmaMapMemory and vmaUnmapMemory,
// to use them set .allocation_info_out = nullptr otherwise VMA_ALLOCATION_CREATE_MAPPED_BIT
// will be added to the vma_allocation_create_flags.
struct StagingBufferMemoryCreateInfoDefaults
{
  static constexpr ptrdiff_t use_temporary_allocation_info_magic = 0xAFAFAFAFUL;

  vk::BufferUsageFlags        usage                           = vk::BufferUsageFlagBits::eTransferSrc;
  vk::MemoryPropertyFlags     properties                      = vk::MemoryPropertyFlagBits::eHostVisible;
  VmaAllocationCreateFlags    vma_allocation_create_flags     = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
  VmaMemoryUsage              vma_memory_usage                = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo*          allocation_info_out             = reinterpret_cast<VmaAllocationInfo*>(use_temporary_allocation_info_magic);
};

// Note it is better to use VMA_ALLOCATION_CREATE_MAPPED_BIT, see for example
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html#:~:text=Staging%20copy%20for%20upload
// and Eearslya (Vulkan discord): I suggest avoiding vmaMapMemory and vmaUnmapMemory entirely.
//
// As such, one should use a StagingBuffer by passing a memory_create_info that has
// VMA_ALLOCATION_CREATE_MAPPED_BIT added to vma_allocation_create_flags and has
// allocation_info_out set to a local VmaAllocationInfo object. A pointer to the
// mapped memory then can be obtained from allocation_info_out->pMappedData.
//
struct StagingBuffer : Buffer
{
  using MemoryCreateInfo = StagingBufferMemoryCreateInfoDefaults;

  StagingBuffer() = default;

 private:
  static VmaAllocationCreateFlags allocation_create_flags(MemoryCreateInfo const& memory_create_info)
  {
    // Unless allocation_info_out is set to nullptr, add VMA_ALLOCATION_CREATE_MAPPED_BIT.
    if (memory_create_info.allocation_info_out)
      return memory_create_info.vma_allocation_create_flags | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    // You set VMA_ALLOCATION_CREATE_MAPPED_BIT and set allocation_info_out to nullptr. Don't do that.
    ASSERT(!(memory_create_info.vma_allocation_create_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT));
    return memory_create_info.vma_allocation_create_flags;
  }

  static VmaAllocationInfo* allocation_info_ptr(MemoryCreateInfo const& memory_create_info, VmaAllocationInfo& vma_allocation_info_tmp)
  {
    if (memory_create_info.allocation_info_out == reinterpret_cast<VmaAllocationInfo*>(MemoryCreateInfo::use_temporary_allocation_info_magic))
      return &vma_allocation_info_tmp;
    return memory_create_info.allocation_info_out;
  }

 public:
  // Construct a StagingBuffer using VMA_ALLOCATION_CREATE_MAPPED_BIT, automatically filling in m_pointer.
  StagingBuffer(LogicalDevice const* logical_device, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix),
      MemoryCreateInfo memory_create_info = {},
      VmaAllocationInfo vma_allocation_info_tmp = {}) :
    Buffer(logical_device, size, {
        .usage                       = memory_create_info.usage,
        .properties                  = memory_create_info.properties,
        .vma_allocation_create_flags = allocation_create_flags(memory_create_info),
        .vma_memory_usage            = memory_create_info.vma_memory_usage,
        .allocation_info_out         = allocation_info_ptr(memory_create_info, vma_allocation_info_tmp) }
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_pointer(allocation_info_ptr(memory_create_info, vma_allocation_info_tmp) ? allocation_info_ptr(memory_create_info, vma_allocation_info_tmp)->pMappedData : nullptr) { }

  void* m_pointer;
};

} // namespace vulkan::memory
