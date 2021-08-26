#include "sys.h"
#include <vulkan/vulkan.hpp>    // This header must be included before including Window.h, because this TU uses createSurface.
#include "vulkan/HelloTriangleSwapChain.h"
#include "Window.h"
#include "Application.h"

void Window::create_surface()
{
  DoutEntering(dc::vulkan, "Window::create_surface() [" << (void*)this << "]");
  vk::Instance vh_instance{static_cast<Application&>(application()).vh_instance()};
  m_uh_surface = vulkan::unique_handle<vk::SurfaceKHR>(
      get_glfw_window().createSurface(vh_instance),
      [vh_instance](auto& surface){ vh_instance.destroySurfaceKHR(surface); }
      );
}

Window::~Window()
{
  DoutEntering(dc::vulkan, "Window::~Window() [" << (void*)this << "]");
}

void Window::createCommandBuffers(vulkan::Device const& device, vulkan::Pipeline* pipeline)
{
  // Currently we are assuming this function is only called once.
  ASSERT(m_command_buffers.empty());

  vk::CommandBufferAllocateInfo alloc_info;
  alloc_info.level = vk::CommandBufferLevel::ePrimary;
  alloc_info.commandPool = device.vh_command_pool();
  ASSERT(m_swap_chain->image_count() > 0);
  alloc_info.commandBufferCount = static_cast<uint32_t>(m_swap_chain->image_count());

  m_command_buffers = device->allocateCommandBuffers(alloc_info);

  for (size_t i = 0; i < m_command_buffers.size(); ++i)
  {
    vk::CommandBufferBeginInfo begin_info;
    m_command_buffers[i].begin(begin_info);

    vk::RenderPassBeginInfo render_pass_info;
    render_pass_info.renderPass = m_swap_chain->vh_render_pass();
    render_pass_info.framebuffer = m_swap_chain->vh_frame_buffer(vulkan::SwapChainIndex{i});    // FIXME: mixture of SwapChainIndex with command buffer index.

    render_pass_info.renderArea.offset = vk::Offset2D{ 0, 0 };
    render_pass_info.renderArea.extent = m_swap_chain->get_swap_chain_extent();

    std::array<vk::ClearValue, 2> clear_values;
    clear_values[0].color.setFloat32({ 0.1f, 0.1f, 0.1f, 1.0f });
    clear_values[1].depthStencil.setDepth(1.0f).setStencil(0);
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    m_command_buffers[i].beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

    pipeline->bind(m_command_buffers[i]);
    m_command_buffers[i].draw(3, 1, 0, 0);

    m_command_buffers[i].endRenderPass();
    m_command_buffers[i].end();
  }
}

void Window::create_swap_chain(vulkan::Device const& device, vulkan::Queue graphics_queue, vulkan::Queue present_queue)
{
  m_swap_chain = std::make_unique<vulkan::HelloTriangleSwapChain>(device);
  auto extent = get_glfw_window().getSize();
  m_swap_chain->setup({ static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) }, graphics_queue, present_queue, *m_uh_surface);
}

void Window::draw_frame()
{
  vulkan::SwapChainIndex image_index{m_swap_chain->acquireNextImage()};
  m_swap_chain->submitCommandBuffers(m_command_buffers[image_index.get_value()], image_index);  // FIXME: mixture of SwapChainIndex and command buffer index.
}
