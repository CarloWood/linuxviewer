#pragma once

#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace task {
class VulkanWindow;
} // namespace task

namespace vulkan {

class Swapchain;
using SwapchainIndex = utils::VectorIndex<Swapchain>;

class Swapchain
{
 public:
  using images_type = utils::Vector<vk::Image, SwapchainIndex>;
  using image_views_type = utils::Vector<vk::UniqueImageView, SwapchainIndex>;

 private:
  std::array<uint32_t, 2>       m_queue_family_indices; // Pointed to by m_create_info.
  vk::SwapchainCreateInfoKHR    m_create_info;          // A cached copy of the create info object, initialized in prepare and reused in recreate.
  vk::UniqueSwapchainKHR        m_swapchain;
  vk::Format                    m_format;
  images_type                   m_vhv_images;
  image_views_type              m_image_views;
  SwapchainIndex                m_swapchain_end;        // The actual number of swap chain images.
  vk::Extent2D                  m_extent;
  vk::PresentModeKHR            m_present_mode;
  vk::ImageUsageFlags           m_usage_flags;
  bool                          m_can_render;

 public:
  Swapchain() : m_format(vk::Format::eUndefined), m_can_render(false) { DoutEntering(dc::vulkan, "Swapchain::Swapchain() [" << this << "]"); }
  ~Swapchain() { DoutEntering(dc::vulkan, "Swapchain::~Swapchain() [" << this << "]"); }

  void prepare(task::VulkanWindow const* owning_window,
      vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode);

  void recreate(task::VulkanWindow const* owning_window, vk::Extent2D window_extent);

  bool can_render() const
  {
    return m_can_render;
  }
};

} // namespace vulkan
