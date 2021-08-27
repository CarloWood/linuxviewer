#pragma once

#include "GUI_glfw3/gui_Window.h"
#include "vulkan/Queue.h"
#include "vulkan/unique_handle.h"
#include "vulkan/Swapchain.h"
#include "utils/InsertExtraInitialization.h"
#include <memory>

namespace vulkan {
class Pipeline;
class Device;
class HelloTriangleSwapchain;
} // namespace vulkan

class Window : public gui::Window
{
 private:
  vulkan::unique_handle<vk::SurfaceKHR> m_uh_surface;
  vulkan::Swapchain m_swapchain2;

  // This had to be moved here for now...
  std::unique_ptr<vulkan::HelloTriangleSwapchain> m_swapchain;
  std::vector<vk::CommandBuffer> m_command_buffers;             // The vulkan command buffers that this application uses.

 public:
  using gui::Window::Window;
  ~Window();

  void create_swapchain(vulkan::Device const& device, vulkan::Queue graphics_queue, vulkan::Queue present_queue);
  void createCommandBuffers(vulkan::Device const& device, vulkan::Pipeline* pipeline);
  void draw_frame();

#if 0
  vk::Extent2D extent() const
  {
    auto extent = get_glfw_window().getSize();
    return { static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) };
  }
#endif

  // Accessors.
  vk::SurfaceKHR vh_surface() const { return *m_uh_surface; }
  vulkan::HelloTriangleSwapchain const* swapchain_ptr() const { return m_swapchain.get(); }

 private:
  void create_surface();

  // When constructing a Window.
  INSERT_EXTRA_INITIALIZATION { create_surface(); };
};
