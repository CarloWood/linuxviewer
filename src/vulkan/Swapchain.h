#pragma once

#include "ResourceState.h"
#include "ImageKind.h"
#include "Attachment.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#include <thread>
#include <deque>
#include <optional>

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan {

namespace rendergraph {
class RenderPass;
} // namespace rendergraph

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
    .aspectMask = vk::ImageAspectFlagBits::eColor, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = VK_REMAINING_ARRAY_LAYERS
  };

 private:
  std::array<uint32_t, 2>   m_queue_family_indices;     // Pointed to by m_kind.
  vk::Extent2D              m_extent;                   // Copy of the last (non-zero) extent of the owning_window that was passed to recreate.
  SwapchainKind             m_kind;                     // Static data (initialized during prepare) that describe the swapchain.
  uint32_t                  m_min_image_count;          // The minimum number of swapchain images that we (will) request(ed).
  vk::UniqueSwapchainKHR    m_swapchain;
  images_type               m_vhv_images;               // A vector of swapchain images.
  resources_type            m_resources;                // A vector of corresponding image views and semaphores.
  SwapchainIndex            m_current_index;            // The index of the current image and resources.
  vk::UniqueSemaphore       m_acquire_semaphore;        // Semaphore used to acquire the next image.
  vk::PresentModeKHR        m_present_mode;
  // prepare:
  std::optional<Attachment> m_presentation_attachment;  // The presentation attachment ("optional" because it is initialized during prepare).
  // RenderGraph::generate:
  rendergraph::RenderPass*  m_render_pass_output_sink = nullptr;        // The render pass that stores to presentation attachment as a sink.
  vk::UniqueRenderPass      m_render_pass;                              // The render pass that writes to m_frame_buffer.
  vk::UniqueFramebuffer     m_framebuffer;                              // The imageless framebuffer used for rendering to a swapchain image (also see https://i.stack.imgur.com/K0NRD.png).

 public:
  Swapchain() { DoutEntering(dc::vulkan, "Swapchain::Swapchain() [" << this << "]"); }
  ~Swapchain() { DoutEntering(dc::vulkan, "Swapchain::~Swapchain() [" << this << "]"); }

  void prepare(task::SynchronousWindow* owning_window, vk::ImageUsageFlags const selected_usage, vk::PresentModeKHR const selected_present_mode
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& ambifix));

  // Set and get the rendergraph node that writes to the presentation attachment.
  void set_render_pass_output_sink(rendergraph::RenderPass* sink) { m_render_pass_output_sink = sink; }
  rendergraph::RenderPass* render_pass_output_sink() const { ASSERT(m_render_pass_output_sink); return m_render_pass_output_sink; }

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

  Attachment const& presentation_attachment() const
  {
    return m_presentation_attachment.value();
  }

  void set_clear_value_presentation_attachment(ClearValue clear_value)
  {
    m_presentation_attachment.value().set_clear_value(clear_value);
  }

  SwapchainKind const& kind() const
  {
    return m_kind;
  }

  ImageKind const& image_kind() const
  {
    return m_kind.image();
  }

  ImageViewKind const& image_view_kind() const
  {
    return m_kind.image_view();
  }

  vk::Extent2D const& extent() const
  {
    return m_extent;
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
};

} // namespace vulkan
