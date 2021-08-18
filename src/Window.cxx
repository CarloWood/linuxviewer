#include "sys.h"
#include <vulkan/vulkan.hpp>    // This header must be included before including Window.h, because this TU uses createSurface.
#include "Window.h"
#include "Application.h"

void Window::create_surface()
{
  DoutEntering(dc::vulkan, "Window::create_surface() [" << (void*)this << "]");
  vk::Instance vulkan_instance{static_cast<Application&>(application()).vulkan_instance()};
  m_surface = vulkan::unique_handle<vk::SurfaceKHR>(
      get_glfw_window().createSurface(vulkan_instance),
      [vulkan_instance](auto& surface){ vulkan_instance.destroySurfaceKHR(surface); }
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

  vk::CommandBufferAllocateInfo allocInfo;
  allocInfo.level = vk::CommandBufferLevel::ePrimary;
  allocInfo.commandPool = device.get_command_pool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_swap_chain->imageCount());

  m_command_buffers = device->allocateCommandBuffers(allocInfo);

  for (int i = 0; i < m_command_buffers.size(); ++i)
  {
    vk::CommandBufferBeginInfo beginInfo;
    m_command_buffers[i].begin(beginInfo);

    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.renderPass = m_swap_chain->getRenderPass();
    renderPassInfo.framebuffer = m_swap_chain->getFrameBuffer(i);

    renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
    renderPassInfo.renderArea.extent = m_swap_chain->getSwapChainExtent();

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color.setFloat32({ 0.1f, 0.1f, 0.1f, 1.0f });
    clearValues[1].depthStencil.setDepth(1.0f).setStencil(0);
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    m_command_buffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

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
  m_swap_chain->setup({ static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) }, graphics_queue, present_queue, m_surface);
}

void Window::drawFrame()
{
  uint32_t imageIndex = m_swap_chain->acquireNextImage();

  m_swap_chain->submitCommandBuffers(m_command_buffers[imageIndex], imageIndex);
}
