#pragma once

#include "Defaults.h"

#define VULKAN_KIND_PRINT_ON_MEMBERS \
  void print_on(std::ostream& os) const \
  { \
    os << '{'; \
    print_members(os); \
    os << '}'; \
  } \
  void print_members(std::ostream& os) const;

#ifndef CWDEBUG
#define VULKAN_KIND_DEBUG_MEMBERS
#else
#define VULKAN_KIND_DEBUG_MEMBERS VULKAN_KIND_PRINT_ON_MEMBERS
#endif

namespace vulkan {

class ImageKind
{
 public:
  vk::ImageType const image_type = vk::ImageType::e2D;
  vk::Format const format = vk::Format::eB8G8R8A8Srgb;                                  // Also most prefered format in Swapchain.cxx:choose_surface_format
  uint32_t const mip_levels = 1;                                                        // Also vk_defaults::ImageSubresourceRange
  uint32_t const array_layers = 1;                                                      // Also vk_defaults::ImageSubresourceRange
  vk::SampleCountFlagBits const samples = vk::SampleCountFlagBits::e1;
  vk::ImageTiling const tiling = vk::ImageTiling::eOptimal;
  vk::ImageUsageFlags const usage = vk::ImageUsageFlagBits::eColorAttachment;
  vk::SharingMode const sharing_mode = vk::SharingMode::eExclusive;
  uint32_t m_queue_family_index_count = {};
  uint32_t const* m_queue_family_indices = {};
  vk::ImageLayout const initial_layout = vk::ImageLayout::eColorAttachmentOptimal;

  VULKAN_KIND_DEBUG_MEMBERS
};

} // namespace vulkan

#undef VULKAN_KIND_PRINT_ON_MEMBERS
#undef VULKAN_KIND_DEBUG_MEMBERS
