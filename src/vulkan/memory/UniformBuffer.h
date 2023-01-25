#pragma once

#include "Buffer.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif
#include "debug.h"

namespace vulkan::memory {

// These are the defaults for a UniformBuffer.
struct UniformBufferMemoryCreateInfoDefaults
{
  static constexpr ptrdiff_t use_temporary_allocation_info_magic = 0xAFAFAFAFUL;

  vk::BufferUsageFlags        usage                           = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
  vk::MemoryPropertyFlags     properties                      = vk::MemoryPropertyFlagBits::eHostVisible;
  VmaAllocationCreateFlags    vma_allocation_create_flags     = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                                                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                                                VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VmaMemoryUsage              vma_memory_usage                = VMA_MEMORY_USAGE_AUTO;
  VmaAllocationInfo*          allocation_info_out             = reinterpret_cast<VmaAllocationInfo*>(use_temporary_allocation_info_magic);
};

struct UniformBuffer : Buffer
{
  using MemoryCreateInfo = UniformBufferMemoryCreateInfoDefaults;

 private:
  void* const m_pointer;

  static VmaAllocationInfo* allocation_info_ptr(MemoryCreateInfo const& memory_create_info, VmaAllocationInfo& vma_allocation_info_tmp)
  {
    if (memory_create_info.allocation_info_out == reinterpret_cast<VmaAllocationInfo*>(MemoryCreateInfo::use_temporary_allocation_info_magic))
      return &vma_allocation_info_tmp;
    // You are not allowed to set .allocation_info_out to nullptr.
    ASSERT(memory_create_info.allocation_info_out);
    return memory_create_info.allocation_info_out;
  }

 public:
  UniformBuffer() = default;

  UniformBuffer(LogicalDevice const* logical_device, vk::DeviceSize size
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix),
      MemoryCreateInfo memory_create_info = {},
      VmaAllocationInfo vma_allocation_info_tmp = {}) :
    Buffer(logical_device, size,
        { memory_create_info.usage,
          memory_create_info.properties,
          memory_create_info.vma_allocation_create_flags,
          memory_create_info.vma_memory_usage,
          allocation_info_ptr(memory_create_info, vma_allocation_info_tmp) }
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_pointer(allocation_info_ptr(memory_create_info, vma_allocation_info_tmp) ? allocation_info_ptr(memory_create_info, vma_allocation_info_tmp)->pMappedData : nullptr)
  {
    // VMA_ALLOCATION_CREATE_MAPPED_BIT must remain set. In fact, don't change .vma_allocation_create_flags at all?
    ASSERT((memory_create_info.vma_allocation_create_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT));
#if CW_DEBUG
    vk::MemoryPropertyFlags memory_property_flags;
    logical_device->get_allocation_memory_properties(m_vh_allocation, memory_property_flags);
    // See https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html#:~:text=Advanced%20data%20uploading
    // If this flag is not set then a staging buffer is necessary!
    ASSERT((memory_property_flags & vk::MemoryPropertyFlagBits::eHostVisible));
#endif
  }

  // Accessor.
  void* pointer() const
  {
    // Did you forget to call 'create' on your UniformBuffer?
    ASSERT(this);
    return m_pointer;
  }

#if CW_DEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::memory
