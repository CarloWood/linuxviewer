#pragma once

#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace task {
class VulkanWindow;
} // namespace task

namespace vulkan {

class Swapchain;
using SwapchainIndex = utils::VectorIndex<Swapchain>;
#ifdef CWDEBUG
class AmbifixOwner;
#endif

class Swapchain
{
 public:
  using images_type = utils::Vector<vk::Image, SwapchainIndex>;
  using image_views_type = utils::Vector<vk::UniqueImageView, SwapchainIndex>;

 private:
  std::array<uint32_t, 2>       m_queue_family_indices; // Pointed to by m_create_info.
  vk::SwapchainCreateInfoKHR    m_create_info;          // A cached copy of the create info object, initialized in prepare and reused in recreate.
                                                        // Contains a copy of the last (non-zero) extent of the owning_window that was passed to recreate.
  vk::UniqueSwapchainKHR        m_swapchain;
  images_type                   m_vhv_images;           // A vector of swapchain images.
  image_views_type              m_image_views;          // A vector of corresponding image views.
  SwapchainIndex                m_swapchain_end;        // The actual number of swap chain images.
  vk::PresentModeKHR            m_present_mode;
  vk::UniqueRenderPass          m_render_pass;          // The render pass that writes to m_frame_buffer.
  vk::UniqueFramebuffer         m_framebuffer;          // The imageless framebuffer used for rendering to a swapchain image (also see https://i.stack.imgur.com/K0NRD.png).

 public:
  Swapchain() { DoutEntering(dc::vulkan, "Swapchain::Swapchain() [" << this << "]"); }
  ~Swapchain() { DoutEntering(dc::vulkan, "Swapchain::~Swapchain() [" << this << "]"); }

  void prepare(task::VulkanWindow const* owning_window,
      vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));

  void set_render_pass(vk::UniqueRenderPass&& render_pass)
  {
    // This function should only be called once.
    ASSERT(!m_render_pass);
    m_render_pass = std::move(render_pass);
  }

  void recreate_swapchain_images(task::VulkanWindow const* owning_window, vk::Extent2D window_extent
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));
  void recreate_swapchain_framebuffer(task::VulkanWindow const* owning_window
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));
  void recreate(task::VulkanWindow const* owning_window, vk::Extent2D window_extent
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix))
  {
    recreate_swapchain_images(owning_window, window_extent
        COMMA_CWDEBUG_ONLY(ambifix));
    recreate_swapchain_framebuffer(owning_window
        COMMA_CWDEBUG_ONLY(ambifix));
  }

  vk::Format format() const
  {
    return m_create_info.imageFormat;
  }

  vk::ImageUsageFlags usage() const
  {
    return m_create_info.imageUsage;
  }

  vk::Extent2D const& extent() const
  {
    return m_create_info.imageExtent;
  }

  image_views_type const& image_views() const
  {
    return m_image_views;
  }

#ifdef CWDEBUG
  images_type const& images() const
  {
    return m_vhv_images;
  }
#endif

  vk::SwapchainKHR operator*()
  {
    return *m_swapchain;
  }

  vk::RenderPass vh_render_pass() const
  {
    return *m_render_pass;
  }

  vk::Framebuffer vh_framebuffer() const
  {
    return *m_framebuffer;
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
