#include "sys.h"
#include "gui_Application.h"
#include "gui_Window.h"          // This includes GLFW/glfw3.h.
#include <vector>
#include <stdexcept>
#include <thread>
#include "debug.h"

namespace glfw3 {

// The GLFW error callback that we use.
void error_callback(int errorCode_, const char* what_);

namespace gui {

std::once_flag Application::s_main_instance;

Application::Application(std::string const& application_name) : m_application_name(application_name), m_library(glfw::init()), m_main_window(nullptr)
{
  DoutEntering(dc::notice, "gui::Application::Application(\"" << application_name << "\")");

#ifdef CWDEBUG
  glfwSetErrorCallback(error_callback);
  // These are warning level, so always turn them on.
  if (!DEBUGCHANNELS::dc::glfw.is_on())
    DEBUGCHANNELS::dc::glfw.on();
#endif

#if 0
  // Initialize application.
  Glib::signal_idle().connect(sigc::mem_fun(*this, &Application::on_gui_idle));
#endif
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
  quit();       // Make the GUI main loop terminate.

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

void Application::closeEvent(Window* window)
{
  DoutEntering(dc::notice, "Application::closeEvent()");
  quit();       // Make the GUI main loop terminate.
}

} // namespace gui
} // namespace glfw3

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct glfw("GLFW");
NAMESPACE_DEBUG_CHANNELS_END
#endif
