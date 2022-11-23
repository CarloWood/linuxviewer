#pragma once

#include "Defaults.h"
#include "vk_utils/format.h"
#include "utils/Badge.h"
#include "debug/VULKAN_KIND_DEBUG_MEMBERS.h"

// The *KindPOD structs are intended to be temporary constructed using C++20 designated initializers.
// For example,
//
//   static vulkan::ImageKind const sample_image_kind({
//     .format = vk::Format::eR8G8B8A8Unorm,
//     .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
//   });
//
// Therefore the *Kind constructors take *KindPOD parameters by value.
// Other *Kind objects are passed as const&.

namespace vulkan {

class PresentationSurface;
class Swapchain;
class SwapchainKind;

// Contains all members of ImageCreateInfo except extent.
struct ImageKindPOD
{
  vk::ImageCreateFlags    flags = {};
  vk::ImageType           image_type = vk::ImageType::e2D;                      // Default matches vk::Extent2D.
  vk::Format              format = vk::Format::eB8G8R8A8Srgb;                   // Also most prefered format in Swapchain.cxx:choose_surface_format.
  uint32_t                mip_levels = 1;                                       // Also vk_defaults::ImageSubresourceRange.
  uint32_t                array_layers = 1;                                     // Also vk_defaults::ImageSubresourceRange.
  vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;                // This must be the default (we rely on that by not specifying it when it needs to be e1).
  vk::ImageTiling         tiling = vk::ImageTiling::eOptimal;
  vk::ImageUsageFlags     usage = vk::ImageUsageFlagBits::eColorAttachment;
  vk::SharingMode         sharing_mode = vk::SharingMode::eExclusive;
  uint32_t                m_queue_family_index_count = {};
  uint32_t const*         m_queue_family_indices = {};
  vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;         // Use eUndefined to automatically: use final_layout if presented (and keep it at undefined when discarded).
};

#if CW_DEBUG
extern bool operator==(ImageKindPOD const& lhs, ImageKindPOD const& rhs);
#endif

class ImageKind
{
 private:
  ImageKindPOD m_data;

 public:
  constexpr ImageKind(ImageKindPOD data) : m_data(data) { }

  // Convert into an ImageCreateInfo.
  vk::ImageCreateInfo operator()(uint32_t extent, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined) const
  {
    // Only use uint32_t together with an ImageKind that has image_type set to vk::ImageType::e1D.
    ASSERT(m_data.image_type == vk::ImageType::e1D);
    return get_create_info({ extent, 1, 1 }, initial_layout);
  }

  // Convert into an ImageCreateInfo.
  vk::ImageCreateInfo operator()(vk::Extent2D extent, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined) const
  {
    // Only use vk::Extent2D together with an ImageKind that has image_type set to vk::ImageType::e2D.
    ASSERT(m_data.image_type == vk::ImageType::e2D);
    return get_create_info({ extent.width, extent.height, 1 }, initial_layout);
  }

  // Convert into an ImageCreateInfo.
  vk::ImageCreateInfo operator()(vk::Extent3D const& extent, vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined) const
  {
    // Only use vk::Extent3D together with an ImageKind that has image_type set to vk::ImageType::e3D.
    ASSERT(m_data.image_type == vk::ImageType::e3D);
    return get_create_info(extent, initial_layout);
  }

  // Accessor.
  ImageKindPOD const* operator->() const { return &m_data; }

 private:
  vk::ImageCreateInfo get_create_info(vk::Extent3D const& extent, vk::ImageLayout initial_layout) const;

 public:
#if CW_DEBUG
  bool operator==(ImageKind const& rhs) const { return m_data == rhs.m_data; }
#endif

  VULKAN_KIND_DEBUG_MEMBERS
};

// Contains all members of SwapchainCreateInfoKHR that are not already in ImageKindPOD,
// except surface, minImageCount, oldSwapchain.
struct SwapchainKindPOD
{
  vk::SwapchainCreateFlagsKHR flags = {};
  vk::ColorSpaceKHR image_color_space = {};                                                     // Overwritten in Swapchain::prepare.
  vk::SurfaceTransformFlagBitsKHR pre_transform = {};                                           // Overwritten in Swapchain::prepare.
  vk::CompositeAlphaFlagBitsKHR composite_alpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  vk::PresentModeKHR present_mode = {};                                                         // Overwritten in Swapchain::prepare.
  vk::Bool32 clipped = VK_TRUE;
};

// Contains all members of ImageViewCreateInfo except image.
struct ImageViewKindPOD
{
  vk::ImageViewCreateFlags  flags             = {};
  vk::ImageViewType         view_type         = VULKAN_HPP_NAMESPACE::ImageViewType::e2D;
  vk::Format                format            = VULKAN_HPP_NAMESPACE::Format::eUndefined;        // Overwritten with ImageKind::format by default.
  vk::ComponentMapping      components        = {};
  vk::ImageSubresourceRange subresource_range = vk_defaults::ImageSubresourceRange{};
};

#if CW_DEBUG
extern bool operator==(ImageViewKindPOD const& lhs, ImageViewKindPOD const& rhs);
#endif

class ImageViewKind
{
 private:
  ImageViewKindPOD m_data;
  ImageKind const& m_image_kind;

 public:
  ImageViewKind(ImageKind const& image_kind, ImageViewKindPOD data) : m_data(data), m_image_kind(image_kind)
  {
    // Overwrite vk::Format::eUndefined with the format from image_kind.
    m_data.format = choose_format(data.format, image_kind);
    // Overwrite default subresource_range for depth/stencil image.
    switch (image_kind->format)
    {
      case vk::Format::eD16Unorm:
      case vk::Format::eD32Sfloat:
      case vk::Format::eX8D24UnormPack32:
        m_data.subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
        break;
      case vk::Format::eD16UnormS8Uint:
      case vk::Format::eD24UnormS8Uint:
      case vk::Format::eD32SfloatS8Uint:
        m_data.subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        break;
      case vk::Format::eS8Uint:
        m_data.subresource_range.aspectMask = vk::ImageAspectFlagBits::eStencil;
        break;
      default:
        m_data.subresource_range.aspectMask = vk::ImageAspectFlagBits::eColor;
        break;
    }
  }

  void set_POD(utils::Badge<SwapchainKind>, ImageViewKindPOD data) { m_data = data; }

  bool is_color() const { return static_cast<bool>(m_data.subresource_range.aspectMask & vk::ImageAspectFlagBits::eColor); }
  bool is_depth() const { return static_cast<bool>(m_data.subresource_range.aspectMask & vk::ImageAspectFlagBits::eDepth); }
  bool is_stencil() const { return static_cast<bool>(m_data.subresource_range.aspectMask & vk::ImageAspectFlagBits::eStencil); }
  bool is_depth_stencil() const { return is_depth() && is_stencil(); }
  bool is_depth_and_or_stencil() const { return is_depth() || is_stencil(); }

  // Convert into a ImageViewCreateInfo.
  vk::ImageViewCreateInfo operator()(vk::Image vh_image) const;

  // Accessor.
  ImageViewKindPOD const* operator->() const { return &m_data; }
  ImageKind const& image_kind() const { return m_image_kind; }

 private:
  vk::Format choose_format(vk::Format format_in, ImageKind const& image_kind) const
  {
    if (format_in == vk::Format::eUndefined)
      return image_kind->format;
    // The spec about VkImageViewCreateInfo::format:
    //
    //   If image was created with the VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT flag, and if the format
    //   of the image is not multi-planar, format can be different from the image’s format, but
    //   if image was created without the VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT flag
    //   and they are not equal they must be compatible. Image format compatibility is defined
    //   in the Format Compatibility Classes section. Views of compatible formats will have the
    //   same mapping between texel coordinates and memory locations irrespective of the format,
    //   with only the interpretation of the bit pattern changing.
    ASSERT((image_kind->flags & vk::ImageCreateFlagBits::eMutableFormat) && !vk_utils::format_is_multiplane(image_kind->format));
    ASSERT((image_kind->flags & vk::ImageCreateFlagBits::eBlockTexelViewCompatible|vk::ImageCreateFlagBits::eBlockTexelViewCompatibleKHR) ||
        vk_utils::format_is_compatible(image_kind->format, format_in));
    return format_in;
  }

 public:
#if CW_DEBUG
  bool operator==(ImageViewKind const& rhs) const { return m_data == rhs.m_data && m_image_kind == rhs.m_image_kind; }
#endif

  VULKAN_KIND_DEBUG_MEMBERS
};

class SwapchainKind
{
 private:
  SwapchainKindPOD m_data;
  ImageKind m_image_kind;               // Swapchain color attachment kinds.
  ImageViewKind m_image_view_kind;      //

 public:
  // Initialized in Swapchain::prepare using the set* functions below, not by constructor.
  SwapchainKind() : m_image_kind({}), m_image_view_kind(m_image_kind, {}) { }

  // Accessed from Swapchain::prepare.
  SwapchainKind& set(utils::Badge<Swapchain>, SwapchainKindPOD data) { m_data = data; return *this; }
  void set_image_kind(utils::Badge<Swapchain>, ImageKindPOD image_kind);
  void set_image_view_kind(utils::Badge<Swapchain>, ImageViewKindPOD image_view_kind);

  // Convert into a SwapchainCreateInfoKHR.
  vk::SwapchainCreateInfoKHR operator()(vk::Extent2D extent, uint32_t min_image_count, PresentationSurface const& presentation_surface, vk::SwapchainKHR vh_old_swapchain) const;

  // Accessors.
  SwapchainKindPOD const* operator->() const { return &m_data; }
  ImageKind const& image() const { return m_image_kind; }
  ImageViewKind const& image_view() const { return m_image_view_kind; }

  VULKAN_KIND_DEBUG_MEMBERS
};

} // namespace vulkan
