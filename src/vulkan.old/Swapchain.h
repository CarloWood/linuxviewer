#pragma once

#include "Queue.h"
#include "Device.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <vector>

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
  bool m_can_render = false;
  Device const* m_device;
  // prepare
  Queue m_graphics_queue;
  Queue m_present_queue;
  vk::SwapchainCreateInfoKHR m_create_info;
  // create
  vk::Extent2D m_window_extent;
  vk::UniqueSwapchainKHR m_handle;
  utils::Vector<vk::Image, SwapchainIndex> m_vhv_images;
  utils::Vector<vk::UniqueImageView, SwapchainIndex> m_image_views;
  SwapchainIndex m_swapchain_end;                                       // The size of m_vhv_images / m_image_views.

 public:
  void prepare(
      Device const* device,
      vk::Extent2D window_extent,
      Queue graphics_queue,
      Queue present_queue,
      vk::SurfaceKHR vh_surface,
      vk::ImageUsageFlags const selected_usage,
      vk::PresentModeKHR const selected_present_mode);
  void recreate(vk::Extent2D window_extent);

 private:
  static uint32_t get_number_of_images(vk::SurfaceCapabilitiesKHR const& surface_capabilities, uint32_t selected_image_count);
  static vk::SurfaceFormatKHR choose_format(std::vector<vk::SurfaceFormatKHR> const& surface_formats);
  static vk::Extent2D choose_swap_extent(vk::SurfaceCapabilitiesKHR const& surface_capabilities, vk::Extent2D actual_extent);
  static vk::ImageUsageFlags choose_usage_flags(vk::SurfaceCapabilitiesKHR const& surface_capabilities, vk::ImageUsageFlags const selected_usage);
  static vk::SurfaceTransformFlagBitsKHR get_transform(vk::SurfaceCapabilitiesKHR const& surface_capabilities);
  static vk::PresentModeKHR choose_present_mode(std::vector<vk::PresentModeKHR> const& available_present_modes, vk::PresentModeKHR selected_present_mode);
};

}  // namespace vulkan
