#pragma once

#include "memory/Image.h"

namespace vulkan {

// Data collection used for attachments.
struct Attachment : public memory::Image
{
  vk::UniqueImageView m_image_view;

 public:
  // Used to move-assign later.
  Attachment() = default;

  Attachment(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      VmaAllocationCreateFlags vma_allocation_create_flags,
      vk::MemoryPropertyFlagBits memory_property
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    memory::Image(logical_device, extent, image_view_kind, vma_allocation_create_flags, memory_property
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_image_view(logical_device->create_image_view(m_vh_image, image_view_kind
        COMMA_CWDEBUG_ONLY(ambifix(".m_image_view"))))
  {
  }

  // Class is move-only.
  Attachment(Attachment&& rhs) = default;
  Attachment& operator=(Attachment&& rhs) = default;
};

} // namespace vulkan
