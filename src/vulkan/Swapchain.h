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
  images_type                   m_vhv_images;
  image_views_type              m_image_views;
  SwapchainIndex                m_swapchain_end;        // The actual number of swap chain images.
  vk::Extent2D                  m_extent;               // Copy of the last (non-zero) value that was passed to recreate.
  vk::PresentModeKHR            m_present_mode;
  vk::ImageUsageFlags           m_usage_flags;
  bool                          m_can_render;

 public:
  Swapchain() : m_can_render(false) { DoutEntering(dc::vulkan, "Swapchain::Swapchain() [" << this << "]"); }
  ~Swapchain() { DoutEntering(dc::vulkan, "Swapchain::~Swapchain() [" << this << "]"); }

  void prepare(task::VulkanWindow const* owning_window,
      vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode);

  void recreate(task::VulkanWindow const* owning_window, vk::Extent2D window_extent);

  bool can_render() const
  {
    return m_can_render;
  }

  vk::Format format() const
  {
    return m_create_info.imageFormat;
  }

  vk::Extent2D const& extent() const
  {
    return m_extent;
  }

  image_views_type const& image_views() const
  {
    return m_image_views;
  }

  vk::SwapchainKHR operator*()
  {
    return *m_swapchain;
  }

  void set_image_memory_barriers(task::VulkanWindow const* owning_window,
      vk::ImageSubresourceRange const& image_subresource_range,
      vk::ImageLayout current_image_layout,
      vk::AccessFlags current_image_access,
      vk::PipelineStageFlags generating_stages,
      vk::ImageLayout new_image_layout,
      vk::AccessFlags new_image_access,
      vk::PipelineStageFlags consuming_stages) const; // FIXME: use a struct
};

} // namespace vulkan
