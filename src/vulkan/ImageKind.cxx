#include "sys.h"

#include "ImageKind.h"
#include "PresentationSurface.h"
#include "debug/vulkan_print_on.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_list.h"
#ifdef CWDEBUG
#include "debug/debug_ostream_operators.h"
#endif

namespace vulkan {

vk::ImageCreateInfo ImageKind::get_create_info(vk::Extent3D const& extent, vk::ImageLayout initial_layout) const
{
  // Unless sharing mode is eConcurrent, it makes no sense to assign a queue family array (it would be ignored).
  // Either you forgot to set .sharing_mode for this ImageKind or set queue families that won't be used.
  ASSERT(m_data.sharing_mode == vk::SharingMode::eConcurrent ||
      (m_data.m_queue_family_index_count == 0 && m_data.m_queue_family_indices == nullptr));

  return {
    .flags                 = m_data.flags,
    .imageType             = m_data.image_type,
    .format                = m_data.format,
    .extent                = { extent.width, extent.height, extent.depth },
    .mipLevels             = m_data.mip_levels,
    .arrayLayers           = m_data.array_layers,
    .samples               = m_data.samples,
    .tiling                = m_data.tiling,
    .usage                 = m_data.usage,
    .sharingMode           = m_data.sharing_mode,
    .queueFamilyIndexCount = m_data.m_queue_family_index_count,
    .pQueueFamilyIndices   = m_data.m_queue_family_indices,
    .initialLayout         = initial_layout
  };
}

#ifdef CWDEBUG
void ImageKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", image_type:" << m_data.image_type <<
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
#endif

void SwapchainKind::set_image_kind(utils::Badge<Swapchain>, ImageKindPOD image_kind)
{
  // This is what will be used, no matter whatever ImageKind is being passed here,
  // as described by https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#_wsi_swapchain
  // (look for "A presentable image is equivalent to a non-presentable image created with the following").
  vk::ImageCreateFlags image_create_flags{};
  if ((m_data.flags & vk::SwapchainCreateFlagBitsKHR::eSplitInstanceBindRegions))
    image_create_flags |= vk::ImageCreateFlagBits::eSplitInstanceBindRegions;
  if ((m_data.flags & vk::SwapchainCreateFlagBitsKHR::eProtected))
    image_create_flags |= vk::ImageCreateFlagBits::eProtected;
  if ((m_data.flags & vk::SwapchainCreateFlagBitsKHR::eMutableFormat))
    image_create_flags |= vk::ImageCreateFlagBits::eMutableFormat | vk::ImageCreateFlagBits::eExtendedUsageKHR;

  image_kind.flags = image_create_flags;
  image_kind.image_type = vk::ImageType::e2D;
  image_kind.mip_levels = 1;
  image_kind.samples = vk::SampleCountFlagBits::e1;
  image_kind.tiling = vk::ImageTiling::eOptimal;
  image_kind.initial_layout = vk::ImageLayout::ePresentSrcKHR;

  m_image_kind = image_kind;
}

void SwapchainKind::set_image_view_kind(utils::Badge<Swapchain>, ImageViewKindPOD image_view_kind)
{
  m_image_view_kind.set_POD({}, image_view_kind);
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

#ifdef CWDEBUG
void SwapchainKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", image_color_space:" << m_data.image_color_space <<
      ", pre_transform:" << m_data.pre_transform <<
      ", composite_alpha:" << m_data.composite_alpha <<
      ", present_mode:" << m_data.present_mode <<
      ", clipped:" << std::boolalpha << m_data.clipped;
}
#endif

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

#ifdef CWDEBUG
void ImageViewKind::print_members(std::ostream& os) const
{
  os << "flags:" << m_data.flags <<
      ", view_type:" << m_data.view_type <<
      ", format:" << m_data.format <<
      ", components:" << m_data.components <<
      ", subresource_range:" << m_data.subresource_range <<
      ", m_image_kind:" << m_image_kind;
}
#endif

} // namespace vukan
