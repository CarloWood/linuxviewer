#pragma once

#include <vk_mem_alloc.h>
#include "vk_utils/print_flags.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan {

#ifdef CWDEBUG
class Ambifix;
#endif

namespace memory {

class Allocator
{
  VmaAllocator m_handle{};

 private:
  void destroy()
  {
    if (m_handle)
      vmaDestroyAllocator(m_handle);
    m_handle = VK_NULL_HANDLE;
  }

 public:
  Allocator() = default;
  ~Allocator() { destroy(); }

  // Move-only.
  Allocator(Allocator&& rhs) : m_handle(rhs.m_handle) { rhs.m_handle = VK_NULL_HANDLE; }
  Allocator& operator=(Allocator&& rhs) { destroy(); m_handle = rhs.m_handle; rhs.m_handle = VK_NULL_HANDLE; return *this; }

  void create(VmaAllocatorCreateInfo const& vma_allocator_create_info);

  vk::Buffer create_buffer(
      vk::BufferCreateInfo const& buffer_create_info,
      VmaAllocationCreateInfo const& vma_allocation_create_info,
      VmaAllocation* vh_allocation,
      VmaAllocationInfo* allocation_info
      COMMA_CWDEBUG_ONLY(Ambifix const& allocation_name)) const;

  void destroy_buffer(vk::Buffer vh_buffer, VmaAllocation vh_allocation) const
  {
    vmaDestroyBuffer(m_handle, vh_buffer, vh_allocation);
  }

  vk::Image create_image(
      vk::ImageCreateInfo const& image_create_info,
      VmaAllocationCreateInfo const& vma_allocation_create_info,
      VmaAllocation* vh_allocation,
      VmaAllocationInfo* allocation_info
      COMMA_CWDEBUG_ONLY(Ambifix const& allocation_name)) const;

  void destroy_image(vk::Image vh_image, VmaAllocation vh_allocation) const
  {
    vmaDestroyImage(m_handle, vh_image, vh_allocation);
  }

  VmaAllocationInfo get_allocation_info(VmaAllocation vh_allocation) const
  {
    VmaAllocationInfo alloc_info;
    vmaGetAllocationInfo(m_handle, vh_allocation, &alloc_info);
    return alloc_info;
  }

  void* map_memory(VmaAllocation vh_allocation) const
  {
    void* mapped_memory;
    vmaMapMemory(m_handle, vh_allocation, &mapped_memory);
    return mapped_memory;
  }

  void flush_allocation(VmaAllocation vh_allocation, vk::DeviceSize offset, vk::DeviceSize size) const
  {
    vmaFlushAllocation(m_handle, vh_allocation, offset, size);
  }

  void flush_allocations(uint32_t allocation_count, VmaAllocation const* vh_allocations, vk::DeviceSize const* offsets, vk::DeviceSize const* sizes) const
  {
    vmaFlushAllocations(m_handle, allocation_count, vh_allocations, offsets, sizes);
  }

  void unmap_memory(VmaAllocation vh_allocation) const
  {
    vmaUnmapMemory(m_handle, vh_allocation);
  }

  void get_allocation_memory_properties(VmaAllocation vh_allocation, vk::MemoryPropertyFlags& memory_property_flags_out) const
  {
    VkMemoryPropertyFlags memory_property_flags;
    vmaGetAllocationMemoryProperties(m_handle, vh_allocation, &memory_property_flags);
    memory_property_flags_out = vk::MemoryPropertyFlags{memory_property_flags};
  }
};

} // namespace memory
} // namespace vulkan
