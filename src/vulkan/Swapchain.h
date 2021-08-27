#pragma once

// Used everywhere, but originating from here, the following
// abbreviations are being used:
//
// vh_   : Vulkan handle. This a light-weight type; a wrapper around a vulkan handle (which basically is a pointer).
//         The reason for the special naming is because it *looks* like a real type, since the wrapper class has
//         member functions that act on the underlaying object.
// vhv_  : A Vulkan handle vector.

namespace vulkan {

class Swapchain;

// An index for Vectors containing swap chain images and image views.
using SwapchainIndex = utils::VectorIndex<Swapchain>;

// Replacement for HelloTriangleSwapchain.
class Swapchain
{
 private:
  vk::UniqueSwapchainKHR m_handle;
  vk::Format m_image_format = vk::Format::eUndefined;
  utils::Vector<vk::Image, SwapchainIndex> m_vhv_images;
  utils::Vector<vk::UniqueImageView, SwapchainIndex> m_image_views;
  vk::Extent2D m_window_extent;
  vk::PresentModeKHR m_present_mode;
  vk::ImageUsageFlags m_image_usage_flags;
};

}  // namespace vulkan
