#include "sys.h"
#include "Buffer.h"
#include "LogicalDevice.h"

namespace vulkan::memory {

Buffer::Buffer(
    LogicalDevice const* logical_device,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    VmaAllocationCreateFlags vma_allocation_create_flags,
    vk::MemoryPropertyFlagBits memory_property
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) : m_logical_device(logical_device), m_size(size)
{
  // VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
  // Requests possibility to map the allocation.
  // * If you use VMA_MEMORY_USAGE_AUTO or other VMA_MEMORY_USAGE_AUTO* value, you must use this flag to be able to map the allocation. Otherwise, mapping is incorrect.
  // * Declares that mapped memory will only be written sequentially, e.g. using memcpy() or a loop writing number-by-number, never read or accessed randomly,
  //   so a memory type can be selected that is uncached and write-combined.
  ASSERT(!(memory_property & vk::MemoryPropertyFlagBits::eHostVisible) ||
      (vma_allocation_create_flags & VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT));

  if (!(memory_property & vk::MemoryPropertyFlagBits::eHostCoherent))
  {
    // Allocate a multiple of non_coherent_atom_size bytes.
    m_size = ((m_size - 1) / logical_device->non_coherent_atom_size() + 1) * logical_device->non_coherent_atom_size();
  }

  vk::BufferCreateInfo buffer_create_info{
    .size = m_size,
    .usage = usage
  };
  VmaAllocationCreateInfo vma_allocation_create_info{
    .flags = vma_allocation_create_flags,
    .usage = VMA_MEMORY_USAGE_AUTO
  };

  m_vh_buffer = logical_device->create_buffer({}, buffer_create_info, vma_allocation_create_info, &m_vh_allocation COMMA_CWDEBUG_ONLY(ambifix(".m_vh_allocation")));
  DebugSetName(m_vh_buffer, ambifix(".m_vh_buffer"), logical_device);

#ifdef CWDEBUG
  vk::MemoryRequirements buffer_memory_requirements = logical_device->get_buffer_memory_requirements(m_vh_buffer);
  Dout(dc::vulkan, "Allocated buffer " << m_vh_buffer << " with memory requirements: " <<
      print_using(buffer_memory_requirements, logical_device->memory_requirements_printer()) <<
      " and allocation info: " << logical_device->get_allocation_info(m_vh_allocation));
#endif
}

#ifdef CWDEBUG
void Buffer::print_on(std::ostream& os) const
{
  os << "{vh_buffer:" << m_vh_buffer <<
      ", vh_allocation:" << m_vh_allocation <<
      ", size:" << m_size <<
      ", logical_device:" << m_logical_device << '}';
}
#endif

} // namespace memory::vulkan
