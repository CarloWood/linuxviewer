#include "sys.h"

#include "ImageKind.h"
#include "PresentationSurface.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/PrintList.h"
#include "debug/debug_ostream_operators.h"

namespace vulkan {

vk::ImageCreateInfo ImageKind::operator()(vk::Extent2D extent) const
{
  // Unless sharing mode is eConcurrent, it makes no sense to assign a queue family array (it would be ignored).
  // Either you forgot to set .sharing_mode for this ImageKind or set queue families that won't be used.
  ASSERT(m_data.sharing_mode == vk::SharingMode::eConcurrent ||
      (m_data.m_queue_family_index_count == 0 && m_data.m_queue_family_indices == nullptr));

  return {
    .flags                 = m_data.flags,
    .imageType             = vk::ImageType::e2D,
    .format                = m_data.format,
    .extent                = { extent.width, extent.height, 1 },
    .mipLevels             = m_data.mip_levels,
    .arrayLayers           = m_data.array_layers,
    .samples               = m_data.samples,
    .tiling                = m_data.tiling,
    .usage                 = m_data.usage,
    .sharingMode           = m_data.sharing_mode,
    .queueFamilyIndexCount = m_data.m_queue_family_index_count,
    .pQueueFamilyIndices   = m_data.m_queue_family_indices,
    .initialLayout         = m_data.initial_layout
  };
}

void ImageKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", format:" << m_data.format <<
      ", mip_levels:" << m_data.mip_levels <<
      ", array_layers:" << m_data.array_layers <<
      ", samples:" << m_data.samples <<
      ", tiling:" << m_data.tiling <<
      ", usage:" << m_data.usage <<
      ", sharing_mode:" << m_data.sharing_mode;
  if (m_data.sharing_mode == vk::SharingMode::eConcurrent)
    os << vk_utils::print_list(m_data.m_queue_family_indices, m_data.m_queue_family_index_count);
  os << ", initial_layout:" << m_data.initial_layout;
}

vk::SwapchainCreateInfoKHR SwapchainKind::operator()(vk::Extent2D extent, uint32_t min_image_count, PresentationSurface const& presentation_surface, vk::SwapchainKHR vh_old_swapchain) const
{
  return {
    .flags                 = m_data.flags,
    .surface               = presentation_surface.vh_surface(),
    .minImageCount         = min_image_count,
    .imageFormat           = m_image_kind->format,
    .imageColorSpace       = m_data.image_color_space,
    .imageExtent           = extent,
    .imageArrayLayers      = m_image_kind->array_layers,
    .imageUsage            = m_image_kind->usage,
    .imageSharingMode      = m_image_kind->sharing_mode,
    .queueFamilyIndexCount = m_image_kind->m_queue_family_index_count,
    .pQueueFamilyIndices   = m_image_kind->m_queue_family_indices,
    .preTransform          = m_data.pre_transform,
    .compositeAlpha        = m_data.composite_alpha,
    .presentMode           = m_data.present_mode,
    .clipped               = m_data.clipped,
    .oldSwapchain          = vh_old_swapchain
  };
}

void SwapchainKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", image_color_space:" << m_data.image_color_space <<
      ", pre_transform:" << m_data.pre_transform <<
      ", composite_alpha:" << m_data.composite_alpha <<
      ", present_mode:" << m_data.present_mode <<
      ", clipped:" << std::boolalpha << m_data.clipped;
}

vk::ImageViewCreateInfo ImageViewKind::operator()(vk::Image vh_image) const
{
  return {
    .flags            = m_data.flags,
    .image            = vh_image,
    .viewType         = m_data.view_type,
    .format           = m_data.format,
    .components       = m_data.components,
    .subresourceRange = m_data.subresource_range
  };
}

void ImageViewKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", view_type:" << m_data.view_type <<
      ", format:" << m_data.format <<
      ", components:" << m_data.components <<
      ", subresource_range:" << m_data.subresource_range;
}

} // namespace vukan
