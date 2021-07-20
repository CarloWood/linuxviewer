#include "sys.h"
#include "gui_Application.h"
#include "gui_Window.h"          // This includes GLFW/glfw3.h.
#include "vulkan/HelloTriangleSwapChain.h"
#include <vector>
#include <stdexcept>
#include <thread>
#include "debug.h"

namespace glfw3 {

// The GLFW error callback that we use.
void error_callback(int errorCode_, const char* what_);

namespace gui {

std::once_flag Application::s_main_instance;

Application::Application(std::string const& application_name) : m_application_name(application_name), m_library(glfw::init()), m_return_from_main(false), m_main_window(nullptr)
{
  DoutEntering(dc::notice, "gui::Application::Application(\"" << application_name << "\")");

#ifdef CWDEBUG
  glfwSetErrorCallback(error_callback);
  // These are warning level, so always turn them on.
  if (DEBUGCHANNELS::dc::glfw.is_on())
    DEBUGCHANNELS::dc::glfw.on();
#endif

#if 0
  // Initialize application.
  Glib::signal_idle().connect(sigc::mem_fun(*this, &Application::on_gui_idle));
#endif
}

std::shared_ptr<Window> Application::create_main_window(WindowCreateInfo const& create_info)
{
  DoutEntering(dc::notice, "gui::Application::create_window() [NOT IMPLEMENTED]");

  // Only call create_main_window() once.
  ASSERT(!m_main_window);

  // Do one-time initialization.
  std::call_once(s_main_instance, [this]{ on_main_instance_startup(); });

  create_info.apply();  // Call glfw::WindowHints::apply.
  m_main_window = std::make_shared<Window>(this, create_info);

#if 0
  // Make sure that the application runs as long this window is still open.
  add_window(*main_window);
  std::vector<Gtk::Window*> windows = get_windows();
  Dout(dc::notice, "The application has " << windows.size() << " windows.");
  ASSERT(G_IS_OBJECT(main_window->gobj()));

  // Delete the window when it is hidden.
  main_window->signal_hide().connect(sigc::bind<Gtk::Window*>(sigc::mem_fun(*this, &Application::on_window_hide), main_window));

  main_window->show_all();
#endif

  return m_main_window;
}

void Application::createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain)
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

#if 0
void Application::on_window_hide(Gtk::Window* window)
{
  DoutEntering(dc::notice, "Application::on_window_hide(" << window << ")");
  // There is only one window, no?
  ASSERT(window == m_main_window);

  // Hiding the window removed it from the application.
  // Set our pointer to nullptr, just to be sure we won't access it again.
  // Delete the window.
  if (window == m_main_window)
  {
    m_main_window = nullptr;
    delete window;
  }

  // This doesn't really do anything anymore, but well.
  terminate();

  Dout(dc::notice, "Leaving Application::on_window_hide()");
}
#endif

void Application::terminate()
{
  DoutEntering(dc::notice, "gui::Application::terminate()");

  mainloop_quit();

#if 0
  // Gio::Application::quit() will make Gio::Application::run() return,
  // but it's a crude way of ending the program. The window is not removed
  // from the application. Neither the window's nor the application's
  // destructors will be called, because there will be remaining reference
  // counts in both of them. If we want the destructors to be called, we
  // must remove the window from the application. One way of doing this
  // is to hide the window.
  std::vector<Gtk::Window*> windows = get_windows();
  Dout(dc::notice, "Hiding all " << windows.size() << " windows of the application.");
  for (auto window : windows)
    window->hide();
#endif

  // Delete all windows.
  m_main_window.reset();

  Dout(dc::notice, "Leaving Application::terminate()");
}

void Application::mainloop(vulkan::HelloTriangleSwapChain& swap_chain)
{
  DoutEntering(dc::notice|flush_cf, "gui::Application::main()");

  //glfw::makeContextCurrent(*m_main_window);           // Only possible when using GLFW_OPENGL_API or GLFW_OPENGL_ES_API.
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
}

void Application::mainloop_quit()
{
  DoutEntering(dc::notice, "gui::Application::main_quit()");
  m_return_from_main = true;
}

void Application::closeEvent(Window* window)
{
  DoutEntering(dc::notice, "Application::closeEvent()");
  mainloop_quit();
}

} // namespace gui
} // namespace glfw3

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct glfw("GLFW");
NAMESPACE_DEBUG_CHANNELS_END
#endif
