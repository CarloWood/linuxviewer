#pragma once

#include "gui_Window.h"         // Needed for glfw::GlfwLibrary (this includes <glfwpp/glfwpp.h>).
#include "vulkan/HelloTriangleSwapChain.h"
#include "vulkan/HelloTriangleDevice.h"
#include "vulkan/Pipeline.h"
#include "debug.h"
#include <string>
#include <mutex>
#include <memory>
#include <vector>

class AIEngine;
class LinuxViewerMenuBar;

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct glfw;
NAMESPACE_DEBUG_CHANNELS_END
#endif

// This is the GUI implementation that is implemented on top of glfw3.
namespace glfw3 {
namespace gui {

class Application
{
 private:
  static std::once_flag s_main_instance;                // Make sure that m_main_window is only initialized once.
  std::string m_application_name;                       // Cache of what was passed to the constructor.
  glfw::GlfwLibrary m_library;                          // Initialize libglfw.
  bool m_return_from_main;                              // False while the (inner) main loop should keep looping.
  std::shared_ptr<Window> m_main_window;                // Pointer to the main window (same value as what is returned by create_main_window.

 protected:
  std::vector<VkCommandBuffer> m_command_buffers;       // The vulkan command buffers that this application uses.

 protected:
  Application(std::string const& application_name);
  virtual ~Application() { }

 public:
  std::string const& application_name() const { return m_application_name; }

 public:
  std::shared_ptr<Window> create_main_window(WindowCreateInfo const& create_info);
  void createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain);

  //void on_window_hide(Gtk::Window* window);

  //---------------------------------------------------------------------------
  // Everything below is glfw independent interface implemented
  // or used by the derived class LinuxViewerApplication.
  //
 protected:
  bool running() const { return !m_return_from_main; }  // Returns true until main_quit() was called.
  void terminate();                             // Close all windows and cause main() to return.

 protected:
  virtual void on_main_instance_startup() = 0;  // This must be called when this is the main instance of the application.
  virtual bool on_gui_idle() = 0;               // This must be called frequently from the main loop of the GUI.
                                                // Return false when there is nothing to be done anymore.
 public:
  virtual void append_menu_entries(LinuxViewerMenuBar* menubar) = 0;    // Derived class must implement this to add menu buttons and get call backs when they are clicked.
  virtual void drawFrame(vulkan::HelloTriangleSwapChain& swap_chain) = 0;

 public:
  void mainloop(vulkan::HelloTriangleSwapChain& swap_chain);      // Called to run the GUI main loop.
  void mainloop_quit();                         // Called to make the GUI main loop terminate (return from mainloop()).

  void closeEvent(Window* window);              // Called when user clicked (released the mouse button) on the close button of window.
};

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;

namespace menu_keys {
#if 0
  // Make Gtk::Stock IDs available to LinuxViewerApplication::append_menu_entries.
  using namespace Gtk::Stock;
#endif
} // namespace menu_keys
