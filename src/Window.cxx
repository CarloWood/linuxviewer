#include "sys.h"
#include "Window.h"
#include "Application.h"
#include <vulkan/vulkan.hpp>

void Window::create_surface()
{
  DoutEntering(dc::vulkan, "Window::create_surface() [" << (void*)this << "]");
  vk::Instance vulkan_instance{static_cast<Application&>(application()).vulkan_instance()};
  m_surface = get_glfw_window().createSurface(vulkan_instance);
}

Window::~Window()
{
  DoutEntering(dc::vulkan, "Window::~Window() [" << (void*)this << "]");
  if (m_swap_chain_ptr)
    delete m_swap_chain_ptr;
  if (m_surface)
  {
    vk::Instance vulkan_instance{static_cast<Application&>(application()).vulkan_instance()};
    vulkan_instance.destroySurfaceKHR(m_surface);
  }
}

void Window::createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline)
{
  // Currently we are assuming this function is only called once.
  ASSERT(m_command_buffers.empty());

  m_command_buffers.resize(m_swap_chain_ptr->imageCount());
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(m_command_buffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo, m_command_buffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffers!");

  for (int i = 0; i < m_command_buffers.size(); ++i)
  {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(m_command_buffers[i], &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("Failed to begin recording command buffer!");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_swap_chain_ptr->getRenderPass();
    renderPassInfo.framebuffer = m_swap_chain_ptr->getFrameBuffer(i);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swap_chain_ptr->getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
    clearValues[1].depthStencil = { 1.0f, 0 };
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(m_command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    pipeline->bind(m_command_buffers[i]);
    vkCmdDraw(m_command_buffers[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(m_command_buffers[i]);
    if (vkEndCommandBuffer(m_command_buffers[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to record command buffer!");
  }
}

void Window::createSwapChain(vulkan::HelloTriangleDevice& device)
{
  m_swap_chain_ptr = new vulkan::HelloTriangleSwapChain(device);
  auto extent = get_glfw_window().getSize();
  m_swap_chain_ptr->setup({ static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) });
}

void Window::drawFrame()
{
  uint32_t imageIndex;
  auto result = m_swap_chain_ptr->acquireNextImage(&imageIndex);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire swap chain image!");

  result = m_swap_chain_ptr->submitCommandBuffers(&m_command_buffers[imageIndex], &imageIndex);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");
}
