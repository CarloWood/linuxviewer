#pragma once

#include "ResourceState.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <thread>
#include <deque>

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

class Swapchain;
using SwapchainIndex = utils::VectorIndex<Swapchain>;
#ifdef CWDEBUG
class AmbifixOwner;
#endif

class SwapchainResources
{
 private:
  vk::UniqueImageView m_image_view;
  vk::UniqueSemaphore m_image_available_semaphore;
  vk::UniqueSemaphore m_rendering_finished_semaphore;

 public:
  SwapchainResources(vk::UniqueImageView&& image_view, vk::UniqueSemaphore&& image_available_semaphore, vk::UniqueSemaphore rendering_finished_semaphore) :
    m_image_view(std::move(image_view)), m_image_available_semaphore(std::move(image_available_semaphore)), m_rendering_finished_semaphore(std::move(rendering_finished_semaphore)) { }

  vk::ImageView vh_image_view() const
  {
    return *m_image_view;
  }

  vk::Semaphore const* vhp_image_available_semaphore() const
  {
    return &*m_image_available_semaphore;
  }

  vk::Semaphore const* vhp_rendering_finished_semaphore() const
  {
    return &*m_rendering_finished_semaphore;
  }

  void swap_image_available_semaphore_with(vk::UniqueSemaphore& acquire_semaphore)
  {
    m_image_available_semaphore.swap(acquire_semaphore);
  }

  vk::UniqueImageView rescue_image_view()
  {
    return std::move(m_image_view);
  }

  vk::UniqueSemaphore rescue_image_available_semaphore()
  {
    return std::move(m_image_available_semaphore);
  }

  vk::UniqueSemaphore rescue_rendering_finished_semaphore()
  {
    return std::move(m_rendering_finished_semaphore);
  }
};

class Swapchain
{
 public:
  using images_type = utils::Vector<vk::Image, SwapchainIndex>;
  using resources_type = utils::Vector<SwapchainResources, SwapchainIndex>;

  // The subresource range of swapchain images is always just having one mipmap level.
  // Define a default with an aspectMask of eColor.
  static constexpr vk::ImageSubresourceRange s_default_subresource_range = {
    .aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = 1, .layerCount = VK_REMAINING_ARRAY_LAYERS
  };

 private:
  std::array<uint32_t, 2>       m_queue_family_indices; // Pointed to by m_create_info.
  vk::SwapchainCreateInfoKHR    m_create_info;          // A cached copy of the create info object, initialized in prepare and reused in recreate.
                                                        // Contains a copy of the last (non-zero) extent of the owning_window that was passed to recreate.
  vk::UniqueSwapchainKHR        m_swapchain;
  images_type                   m_vhv_images;           // A vector of swapchain images.
  resources_type                m_resources;            // A vector of corresponding image views and semaphores.
  SwapchainIndex                m_current_index;        // The index of the current image and resources.
  vk::UniqueSemaphore           m_acquire_semaphore;    // Semaphore used to acquire the next image.
  vk::PresentModeKHR            m_present_mode;
  vk::UniqueRenderPass          m_render_pass;          // The render pass that writes to m_frame_buffer.
  vk::UniqueFramebuffer         m_framebuffer;          // The imageless framebuffer used for rendering to a swapchain image (also see https://i.stack.imgur.com/K0NRD.png).

 public:
  Swapchain() { DoutEntering(dc::vulkan, "Swapchain::Swapchain() [" << this << "]"); }
  ~Swapchain() { DoutEntering(dc::vulkan, "Swapchain::~Swapchain() [" << this << "]"); }

  void prepare(task::SynchronousWindow* owning_window, vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));

  void set_render_pass(vk::UniqueRenderPass&& render_pass)
  {
    // This function should only be called once.
    ASSERT(!m_render_pass);
    m_render_pass = std::move(render_pass);
  }

  void recreate_swapchain_images(task::SynchronousWindow* owning_window, vk::Extent2D window_extent
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));
  void recreate_swapchain_framebuffer(task::SynchronousWindow const* owning_window
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));
  void recreate(task::SynchronousWindow* owning_window, vk::Extent2D window_extent
      COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));

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

  vk::ImageView vh_current_image_view() const
  {
    return m_resources[m_current_index].vh_image_view();
  }

  vk::Semaphore const* vhp_current_image_available_semaphore() const
  {
    return m_resources[m_current_index].vhp_image_available_semaphore();
  }

  vk::Semaphore const* vhp_current_rendering_finished_semaphore() const
  {
    return m_resources[m_current_index].vhp_rendering_finished_semaphore();
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

  vk::Semaphore vh_acquire_semaphore() const
  {
    return *m_acquire_semaphore;
  }

  SwapchainIndex current_index() const
  {
    return m_current_index;
  }

  void update_current_index(SwapchainIndex new_swapchain_index)
  {
    m_resources[new_swapchain_index].swap_image_available_semaphore_with(m_acquire_semaphore);
    m_current_index = new_swapchain_index;
  }

  void set_image_memory_barriers(
      task::SynchronousWindow const* owning_window,
      ResourceState const& source,
      ResourceState const& destination) const;
};

} // namespace vulkan
