#pragma once

#include "gui_Window.h"         // Needed for glfw::GlfwLibrary (this includes <glfwpp/glfwpp.h>).
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
  std::shared_ptr<Window> m_main_window;                // Pointer to the main window (same value as what is returned by create_main_window).

 protected:
  Application(std::string const& application_name);
  virtual ~Application() { }

 public:
  std::string const& application_name() const { return m_application_name; }

  //void on_window_hide(Gtk::Window* window);

  //---------------------------------------------------------------------------
  // Everything below is glfw independent interface implemented
  // or used by the derived class LinuxViewerApplication.
  //
 protected:
  template<WindowType WindowT>
  std::shared_ptr<WindowT> create_main_window(WindowCreateInfo const& create_info);

  void terminate();                             // Close all windows and cause main() to return.

 protected:
  virtual void on_main_instance_startup() = 0;  // This must be called when this is the main instance of the application.
  virtual bool on_gui_idle() = 0;               // This must be called frequently from the main loop of the GUI.
                                                // Return false when there is nothing to be done anymore.
  virtual void quit() = 0;                      // Called to make the GUI main loop terminate.

 public:
  virtual void append_menu_entries(LinuxViewerMenuBar* menubar) = 0;    // Derived class must implement this to add menu buttons and get call backs when they are clicked.

 public:
  void closeEvent(Window* window);              // Called when user clicked (released the mouse button) on the close button of window.
};

template<WindowType WindowT>
std::shared_ptr<WindowT> Application::create_main_window(WindowCreateInfo const& create_info)
{
  DoutEntering(dc::notice, "gui::Application::create_window() [NOT IMPLEMENTED]");

  // Only call create_main_window() once.
  ASSERT(!m_main_window);

  // Do one-time initialization.
  std::call_once(s_main_instance, [this]{ on_main_instance_startup(); });

  create_info.apply();  // Call glfw::WindowHints::apply.
  auto main_window = std::make_shared<WindowT>(this, create_info);
  m_main_window = main_window;

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

  return main_window;
}

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;

namespace menu_keys {
#if 0
  // Make Gtk::Stock IDs available to LinuxViewerApplication::append_menu_entries.
  using namespace Gtk::Stock;
#endif
} // namespace menu_keys
