#include "sys.h"
#include "Application.h"
#include "WindowCreateInfo.h"
#include "vulkan/HelloTriangleDevice.h"
#include "vulkan/HelloTriangleSwapChain.h"
#include "vulkan/Pipeline.h"
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

  auto main_window = create_main_window(main_window_create_info);

  vulkan::HelloTriangleDevice device(main_window->get_glfw_window());   // The device draws to m_main_window.
  vulkan::HelloTriangleSwapChain swap_chain{device, main_window->getExtent()};
  VkPipelineLayout pipeline_layout;
  createPipelineLayout(device.device(), &pipeline_layout);
  auto pipeline = createPipeline(device.device(), swap_chain, pipeline_layout);
  createCommandBuffers(device, pipeline.get(), swap_chain);

  gui::Application::mainloop(swap_chain);

  // Block until all GPU operations have completed.
  vkDeviceWaitIdle(device.device());
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

bool Application::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}
