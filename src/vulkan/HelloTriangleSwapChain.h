#pragma once

#include "Device.h"
#include "Queue.h"
#include "utils/Vector.h"
#include <string>

namespace vulkan {

class SwapChain;

// An index for Vectors containing swap chain images, image views and their related vk::Semaphore and vk::Fence.
using SwapChainIndex = utils::VectorIndex<SwapChain>;

class HelloTriangleSwapChain
{
 //public:
  static constexpr SwapChainIndex c_max_swap_chain_images{2};     // Requested number of swap chain images.

 private:
  // Constructor
  Device const& m_device;

  // setup
  vk::Extent2D m_window_extent;
  vk::Queue m_vh_graphics_queue;
  vk::Queue m_vh_present_queue;

  // createSwapChain
  vk::Format m_swap_chain_image_format;
  vk::Extent2D m_swap_chain_extent;
  vk::SwapchainKHR m_vh_swap_chain;
  utils::Vector<vk::Image, SwapChainIndex> m_vhv_swap_chain_images;
  SwapChainIndex m_swap_chain_end;                              // The actual number of swap chain images.

  // createImageViews
  utils::Vector<vk::ImageView, SwapChainIndex> m_vhv_swap_chain_image_views;

  // createSyncObjects
  utils::Vector<vk::Semaphore, SwapChainIndex> m_vhv_image_available_semaphores;
  utils::Vector<vk::Semaphore, SwapChainIndex> m_vhv_render_finished_semaphores;
  utils::Vector<vk::Fence, SwapChainIndex> m_vhv_in_flight_fences;
  utils::Vector<vk::Fence, SwapChainIndex> m_vhv_images_in_flight;

  // createRenderPass
  vk::RenderPass m_vh_render_pass;

  // createDepthResources
  utils::Vector<vk::Image, SwapChainIndex> m_vhv_depth_images;
  utils::Vector<vk::DeviceMemory, SwapChainIndex> m_vhv_depth_image_memorys;
  utils::Vector<vk::ImageView, SwapChainIndex> m_vhv_depth_image_views;

  // createFramebuffers
  utils::Vector<vk::Framebuffer, SwapChainIndex> m_vhv_swap_chain_framebuffers;

  size_t m_current_frame = 0;
  SwapChainIndex m_current_swap_chain_index{0};         // m_current_frame % m_vhv_swap_chain_images.size().

 public:
  HelloTriangleSwapChain(Device const& device_ref);
  ~HelloTriangleSwapChain();

  HelloTriangleSwapChain(HelloTriangleSwapChain const&) = delete;
  void operator=(HelloTriangleSwapChain const&) = delete;

  void setup(vk::Extent2D window_extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR vh_surface);

  vk::Framebuffer vh_frame_buffer(SwapChainIndex index) const { return m_vhv_swap_chain_framebuffers[index]; }
  vk::RenderPass vh_render_pass() const { return m_vh_render_pass; }
  vk::ImageView vh_image_view(SwapChainIndex index) { return m_vhv_swap_chain_image_views[index]; }
  size_t image_count() const { return m_vhv_swap_chain_images.size(); }
  vk::Format get_swap_chain_image_format() const { return m_swap_chain_image_format; }
  vk::Extent2D get_swap_chain_extent() const { return m_swap_chain_extent; }
  uint32_t width() const { return m_swap_chain_extent.width; }
  uint32_t height() const { return m_swap_chain_extent.height; }

  float extent_aspect_ratio()
  {
    return static_cast<float>(m_swap_chain_extent.width) / static_cast<float>(m_swap_chain_extent.height);
  }

  uint32_t acquireNextImage();
  void submitCommandBuffers(vk::CommandBuffer const& buffers, SwapChainIndex const swap_chain_index);

 private:
  void createSwapChain(vk::SurfaceKHR vh_surface, Queue graphics_queue, Queue present_queue);
  void createImageViews();
  void createSyncObjects();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();

  // Helper function.
  vk::Format find_depth_format();
};

} // namespace vulkan
