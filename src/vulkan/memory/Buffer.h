#ifndef VULKAN_MEMORY_BUFFER_H
#define VULKAN_MEMORY_BUFFER_H

#include "Allocator.h"

namespace vulkan {
class LogicalDevice;

namespace memory {

// Vulkan Buffer's parameters container class.
struct Buffer
{
  LogicalDevice const* m_logical_device{};              // The associated logical device; only valid when m_vh_buffer is non-null.
  vk::Buffer m_vh_buffer;                               // Vulkan handle to the underlaying buffer, or VK_NULL_HANDLE when no buffer is represented.
  VmaAllocation m_vh_allocation{};                      // The memory allocation used for the buffer; only valid when m_vh_buffer is non-null.
  vk::DeviceSize m_size{};                              // A copy of the size of the buffer (also stored in m_vh_allocation); only valid when m_vh_buffer is non-null.

  Buffer() = default;

  Buffer(LogicalDevice const* logical_device, vk::DeviceSize size, vk::BufferUsageFlags usage,
    VmaAllocationCreateFlags vma_allocation_create_flags, vk::MemoryPropertyFlagBits memory_property
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  Buffer(Buffer&& rhs) : m_logical_device(rhs.m_logical_device), m_vh_buffer(rhs.m_vh_buffer), m_vh_allocation(rhs.m_vh_allocation), m_size(rhs.m_size)
  {
    rhs.m_vh_buffer = VK_NULL_HANDLE;
  }

  ~Buffer()
  {
    destroy();
  }

  Buffer& operator=(Buffer&& rhs)
  {
    destroy();
    m_logical_device = rhs.m_logical_device;
    m_vh_buffer = rhs.m_vh_buffer;
    m_vh_allocation = rhs.m_vh_allocation;
    m_size = rhs.m_size;
    rhs.m_vh_buffer = VK_NULL_HANDLE;
    return *this;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif

  [[gnu::always_inline]] inline void* map_memory();
  [[gnu::always_inline]] inline void unmap_memory();

 private:
  inline void destroy();
};

} // namespace memory
} // namespace vulkan

#endif // VULKAN_MEMORY_BUFFER_H

#ifndef VULKAN_LOGICAL_DEVICE_H
#include "LogicalDevice.h"
#endif

#ifndef VULKAN_MEMORY_BUFFER_H_definitions
#define VULKAN_MEMORY_BUFFER_H_definitions

namespace vulkan::memory {

//inline
void* Buffer::map_memory()
{
  return m_logical_device->map_memory(m_vh_allocation);
}

//inline
void Buffer::unmap_memory()
{
  m_logical_device->unmap_memory(m_vh_allocation);
}

//inline
void Buffer::destroy()
{
  if (AI_UNLIKELY(m_vh_buffer)) // Normally the Buffer will just just default constructed.
    m_logical_device->destroy_buffer({}, m_vh_buffer, m_vh_allocation);
  m_vh_buffer = VK_NULL_HANDLE;
}

} // namespace vulkan::memory

#endif // VULKAN_MEMORY_BUFFER_H_definitions
