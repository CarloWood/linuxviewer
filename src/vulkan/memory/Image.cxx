#include "sys.h"
#include "Image.h"
#include "ImageKind.h"
#include "LogicalDevice.h"

namespace vulkan::memory {

Image::Image(
    LogicalDevice const* logical_device,
    vk::Extent2D extent,
    ImageViewKind const& image_view_kind,
    MemoryCreateInfo memory_create_info
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) : m_logical_device(logical_device)
{
  VmaAllocationCreateInfo vma_allocation_create_info{
    .flags = memory_create_info.vma_allocation_create_flags,
    .usage = memory_create_info.vma_memory_usage
  };

  m_vh_image = logical_device->create_image({}, image_view_kind.image_kind()(extent), vma_allocation_create_info, &m_vh_allocation, memory_create_info.allocation_info_out
      COMMA_CWDEBUG_ONLY(ambifix(".m_vh_allocation")));
  DebugSetName(m_vh_image, ".m_vh_image" + ambifix, logical_device);

#ifdef CWDEBUG
  vk::MemoryRequirements image_memory_requirements = logical_device->get_image_memory_requirements(m_vh_image);
  Dout(dc::vulkan, "Allocated image " << m_vh_image << " with memory requirements: " <<
      print_using(image_memory_requirements, logical_device->memory_requirements_printer()) <<
      " and allocation info: " << logical_device->get_allocation_info(m_vh_allocation));
#endif
}

#ifdef CWDEBUG
void Image::print_on(std::ostream& os) const
{
  os << "{logical_device:" << m_logical_device <<
      ", vh_image:" << m_vh_image <<
      ", vh_allocation:" << m_vh_allocation << '}';
}
#endif

} // namespace memory::vulkan
