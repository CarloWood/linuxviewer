#pragma once

#include "GUI_glfw3/gui_Window.h"
#include "utils/InsertExtraInitialization.h"

namespace vulkan {
class Pipeline;
class HelloTriangleDevice;
class HelloTriangleSwapChain;
} // namespace vulkan

class Window : public gui::Window
{
 private:
  vk::SurfaceKHR m_surface;
  // This had to be moved here for now...
  std::vector<VkCommandBuffer> m_command_buffers;               // The vulkan command buffers that this application uses.
  vulkan::HelloTriangleSwapChain* m_swap_chain_ptr = {};        // The vulkan swap chain for this surface.

 public:
  using gui::Window::Window;
  ~Window();

  void createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline);
  void createSwapChain(vulkan::HelloTriangleDevice& device);
  void drawFrame();

#if 0
  VkExtent2D extent() const
  {
    auto extent = get_glfw_window().getSize();
    return { static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) };
  }
#endif

  // Accessors.
  vk::SurfaceKHR surface() const { return m_surface; }
  vulkan::HelloTriangleSwapChain const* swap_chain_ptr() const { return m_swap_chain_ptr; }

 private:
  void create_surface();

  // When constructing a Window.
  INSERT_EXTRA_INITIALIZATION { create_surface(); };
};
