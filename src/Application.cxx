#include "sys.h"
#include "Application.h"
#include "Window.h"
#include "WindowCreateInfo.h"
#include <vector>

#ifdef CWDEBUG
namespace {
void setupDebugMessenger();
void DestroyDebugUtilsMessengerEXT(vk::Instance instance, VkDebugUtilsMessengerEXT debugMessenger, VkAllocationCallbacks const* pAllocator);
} // namespace
#endif

void Application::createInstance(vulkan::InstanceCreateInfo const& instance_create_info)
{
  if (vulkan::InstanceCreateInfo::s_enableValidationLayers && !vulkan::InstanceCreateInfo::checkValidationLayerSupport())
    throw std::runtime_error("validation layers requested, but not available!");

  Dout(dc::vulkan|continued_cf|flush_cf, "Calling vk::createInstanceUnique()... ");
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif
  m_vulkan_instance = vk::createInstanceUnique(instance_create_info.base());
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
#endif
  Dout(dc::finish, "done (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms)");
  // Mandatory call after creating the vulkan instance.
  m_extension_loader.setup(*m_vulkan_instance);

  // Check that the extensions required by glfw are included.
  vulkan::InstanceCreateInfo::hasGflwRequiredInstanceExtensions(instance_create_info.enabled_extension_names());
}

std::shared_ptr<Window> Application::main_window() const
{
  return std::static_pointer_cast<Window>(m_main_window);
}

void Application::run(int argc, char* argv[],
    WindowCreateInfo const& main_window_create_info
    COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT const& debug_create_info))
{
  DoutEntering(dc::notice|flush_cf|continued_cf, "Application::run(" << argc << ", ");
  // Print the commandline arguments used.
  char const* prefix = "{";
  for (char** arg = argv; *arg; ++arg)
  {
    Dout(dc::continued|flush_cf, prefix << '"' << *arg << '"');
    prefix = ", ";
  }
  Dout(dc::finish|flush_cf, "}, " << main_window_create_info << ", " << debug_create_info << ')');

  // If we get here then this application is the main process and owns the (a) main window.
  // Call the base class methos create_main_window to create the main window.
  create_main_window<Window>(main_window_create_info);

  // Set up the debug messenger so that debug output generated by the validation layers
  // is redirected to debugCallback().
  Debug(m_debug_messenger.setup(*m_vulkan_instance, debug_create_info));

  m_vulkan_device.setup(*m_vulkan_instance, main_window()->surface());   // The device draws to m_main_window.
  // For greater performance, immediately after creating a vulkan device, inform the extension loader.
  m_extension_loader.setup(m_vulkan_device.device());

  vulkan::HelloTriangleSwapChain swap_chain{m_vulkan_device, main_window()->extent()};
  vk::PipelineLayout pipeline_layout = createPipelineLayout(m_vulkan_device);
  auto pipeline = createPipeline(m_vulkan_device.device(), swap_chain, pipeline_layout);
  m_vulkan_device.device().destroyPipelineLayout(pipeline_layout);
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

vk::PipelineLayout Application::createPipelineLayout(vulkan::Device const& device)
{
  vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
  vk::PipelineLayout pipelineLayout =
   device.device().createPipelineLayout(pipelineLayoutInfo);

  return pipelineLayout;
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

#ifdef CWDEBUG

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vkverbose("VKVERBOSE");
channel_ct vkinfo("VKINFO");
channel_ct vkwarning("VKWARNING");
channel_ct vkerror("VKERROR");
NAMESPACE_DEBUG_CHANNELS_END
#endif

void Application::debug_init()
{
  if (!DEBUGCHANNELS::dc::vkerror.is_on())
    DEBUGCHANNELS::dc::vkerror.on();
  if (!DEBUGCHANNELS::dc::vkwarning.is_on() && DEBUGCHANNELS::dc::warning.is_on())
    DEBUGCHANNELS::dc::vkwarning.on();
}

// Default callback function for debug output from vulkan layers.
VkBool32 Application::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData)
{
  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    Dout(dc::vkerror|dc::warning, "\e[31m" << pCallbackData->pMessage << "\e[0m");
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    Dout(dc::vkwarning|dc::warning, "\e[31m" << pCallbackData->pMessage << "\e[0m");
  else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    Dout(dc::vkinfo, pCallbackData->pMessage);
  else
    Dout(dc::vkverbose, pCallbackData->pMessage);

  return VK_FALSE;
}
#endif
