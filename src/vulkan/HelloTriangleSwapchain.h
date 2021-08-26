#pragma once

#include "Device.h"
#include "Queue.h"
#include "utils/Vector.h"
#include <string>

namespace vulkan {

class Swapchain;

// An index for Vectors containing swap chain images, image views and their related vk::Semaphore and vk::Fence.
using SwapchainIndex = utils::VectorIndex<Swapchain>;

class HelloTriangleSwapchain
{
 //public:
  static constexpr SwapchainIndex c_max_swapchain_images{2};     // Requested number of swap chain images.

 private:
  // Constructor
  Device const& m_device;

  // setup
  vk::Extent2D m_window_extent;
  vk::Queue m_vh_graphics_queue;
  vk::Queue m_vh_present_queue;

  // createSwapchain
  vk::Format m_swapchain_image_format;
  vk::Extent2D m_swapchain_extent;
  vk::SwapchainKHR m_vh_swapchain;
  utils::Vector<vk::Image, SwapchainIndex> m_vhv_swapchain_images;
  SwapchainIndex m_swapchain_end;                              // The actual number of swap chain images.

  // createImageViews
  utils::Vector<vk::ImageView, SwapchainIndex> m_vhv_swapchain_image_views;

  // createSyncObjects
  utils::Vector<vk::Semaphore, SwapchainIndex> m_vhv_image_available_semaphores;
  utils::Vector<vk::Semaphore, SwapchainIndex> m_vhv_render_finished_semaphores;
  utils::Vector<vk::Fence, SwapchainIndex> m_vhv_in_flight_fences;
  utils::Vector<vk::Fence, SwapchainIndex> m_vhv_images_in_flight;

  // createRenderPass
  vk::RenderPass m_vh_render_pass;

  // createDepthResources
  utils::Vector<vk::Image, SwapchainIndex> m_vhv_depth_images;
  utils::Vector<vk::DeviceMemory, SwapchainIndex> m_vhv_depth_image_memorys;
  utils::Vector<vk::ImageView, SwapchainIndex> m_vhv_depth_image_views;

  // createFramebuffers
  utils::Vector<vk::Framebuffer, SwapchainIndex> m_vhv_swapchain_framebuffers;

  size_t m_current_frame = 0;
  SwapchainIndex m_current_swapchain_index{0};         // m_current_frame % m_vhv_swapchain_images.size().

 public:
  HelloTriangleSwapchain(Device const& device_ref);
  ~HelloTriangleSwapchain();

  HelloTriangleSwapchain(HelloTriangleSwapchain const&) = delete;
  void operator=(HelloTriangleSwapchain const&) = delete;

  void setup(vk::Extent2D window_extent, Queue graphics_queue, Queue present_queue, vk::SurfaceKHR vh_surface);

  vk::Framebuffer vh_frame_buffer(SwapchainIndex index) const { return m_vhv_swapchain_framebuffers[index]; }
  vk::RenderPass vh_render_pass() const { return m_vh_render_pass; }
  vk::ImageView vh_image_view(SwapchainIndex index) { return m_vhv_swapchain_image_views[index]; }
  size_t image_count() const { return m_vhv_swapchain_images.size(); }
  vk::Format get_swapchain_image_format() const { return m_swapchain_image_format; }
  vk::Extent2D get_swapchain_extent() const { return m_swapchain_extent; }
  uint32_t width() const { return m_swapchain_extent.width; }
  uint32_t height() const { return m_swapchain_extent.height; }

  float extent_aspect_ratio()
  {
    return static_cast<float>(m_swapchain_extent.width) / static_cast<float>(m_swapchain_extent.height);
  }

  uint32_t acquireNextImage();
  void submitCommandBuffers(vk::CommandBuffer const& buffers, SwapchainIndex const swapchain_index);

 private:
  void createSwapchain(vk::SurfaceKHR vh_surface, Queue graphics_queue, Queue present_queue);
  void createImageViews();
  void createSyncObjects();
  void createDepthResources();
  void createRenderPass();
  void createFramebuffers();

  // Helper function.
  vk::Format find_depth_format();
};

} // namespace vulkan
