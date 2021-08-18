#pragma once

#include "GUI_glfw3/gui_Window.h"
#include "vulkan/Queue.h"
#include "vulkan/unique_handle.h"
#include "utils/InsertExtraInitialization.h"
#include <memory>

namespace vulkan {
class Pipeline;
class Device;
class HelloTriangleSwapChain;
} // namespace vulkan

class Window : public gui::Window
{
 private:
  vulkan::unique_handle<vk::SurfaceKHR> m_surface;

  // This had to be moved here for now...
  std::unique_ptr<vulkan::HelloTriangleSwapChain> m_swap_chain;
  std::vector<vk::CommandBuffer> m_command_buffers;             // The vulkan command buffers that this application uses.

 public:
  using gui::Window::Window;
  ~Window();

  void create_swap_chain(vulkan::Device const& device, vulkan::Queue graphics_queue, vulkan::Queue present_queue);
  void createCommandBuffers(vulkan::Device const& device, vulkan::Pipeline* pipeline);
  void drawFrame();

#if 0
  vk::Extent2D extent() const
  {
    auto extent = get_glfw_window().getSize();
    return { static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) };
  }
#endif

  // Accessors.
  vk::SurfaceKHR surface() const { return m_surface; }
  vulkan::HelloTriangleSwapChain const* swap_chain_ptr() const { return m_swap_chain.get(); }

 private:
  void create_surface();

  // When constructing a Window.
  INSERT_EXTRA_INITIALIZATION { create_surface(); };
};
