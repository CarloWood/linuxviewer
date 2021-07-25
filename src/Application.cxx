#include "sys.h"
#include "Application.h"
#include "WindowCreateInfo.h"
#include <vector>

void Application::run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info)
{
  DoutEntering(dc::notice|flush_cf|continued_cf, "Application::run(" << argc << ", ");
  // Print the commandline arguments used.
  char const* prefix = "{";
  for (char** arg = argv; *arg; ++arg)
  {
    Dout(dc::continued|flush_cf, prefix << '"' << *arg << '"');
    prefix = ", ";
  }
  Dout(dc::finish|flush_cf, '}');

  // If we get here then this application is the main process and owns the (a) main window.
  auto main_window = create_main_window(main_window_create_info);

  m_vulkan_device.setup(main_window->get_glfw_window());   // The device draws to m_main_window.
  vulkan::HelloTriangleSwapChain swap_chain{m_vulkan_device, main_window->getExtent()};
  VkPipelineLayout pipeline_layout;
  createPipelineLayout(m_vulkan_device.device(), &pipeline_layout);
  auto pipeline = createPipeline(m_vulkan_device.device(), swap_chain, pipeline_layout);
  createCommandBuffers(m_vulkan_device, pipeline.get(), swap_chain);

#if 0
  //FIXME: is GLEW a vulkan compatible thing?
  if(glewInit() != GLEW_OK)
  {
    throw std::runtime_error("Could not initialize GLEW");
  }
#endif

  // Run the GUI main loop.
  while (running())
  {
    glfw::pollEvents();
    drawFrame(swap_chain);
  }

  // Block until all GPU operations have completed.
  vkDeviceWaitIdle(m_vulkan_device.device());
}

void Application::createPipelineLayout(VkDevice device_handle, VkPipelineLayout* pipelineLayout)
{
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

  if (vkCreatePipelineLayout(device_handle, &pipelineLayoutInfo, nullptr, pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout!");
}

std::unique_ptr<vulkan::Pipeline> Application::createPipeline(VkDevice device_handle, vulkan::HelloTriangleSwapChain const& swap_chain, VkPipelineLayout pipeline_layout_handle)
{
  vulkan::PipelineCreateInfo pipelineConfig{};
  vulkan::Pipeline::defaultPipelineCreateInfo(pipelineConfig, swap_chain.width(), swap_chain.height());
  pipelineConfig.renderPass = swap_chain.getRenderPass();
  pipelineConfig.pipelineLayout = pipeline_layout_handle;
  return std::make_unique<vulkan::Pipeline>(device_handle, SHADERS_DIR "/simple_shader.vert.spv", SHADERS_DIR "/simple_shader.frag.spv", pipelineConfig);
}

void Application::createCommandBuffers(vulkan::Device const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain)
{
  // Currently we are assuming this function is only called once.
  ASSERT(m_command_buffers.empty());

  m_command_buffers.resize(swap_chain.imageCount());
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
    renderPassInfo.renderPass = swap_chain.getRenderPass();
    renderPassInfo.framebuffer = swap_chain.getFrameBuffer(i);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swap_chain.getSwapChainExtent();

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

void Application::drawFrame(vulkan::HelloTriangleSwapChain& swap_chain)
{
  uint32_t imageIndex;
  auto result = swap_chain.acquireNextImage(&imageIndex);
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire swap chain image!");

  result = swap_chain.submitCommandBuffers(&m_command_buffers[imageIndex], &imageIndex);
  if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");
}

void Application::quit()
{
  DoutEntering(dc::notice, "Application::quit()");
  m_return_from_run = true;
}

bool Application::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}
