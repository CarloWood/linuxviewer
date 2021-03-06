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

Application::Application(std::string const& application_name) : m_main_window(nullptr), m_application_name(application_name), m_library(glfw::init())
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

Window* Application::create_window(WindowCreateInfo const& create_info)
{
  DoutEntering(dc::notice, "gui::Application::create_window() [NOT IMPLEMENTED]");

  create_info.apply();  // Call glfw::WindowHints::apply.
  Window* main_window = new Window(this, create_info);

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

  quit(); // Not really necessary, when Gtk::Widget::hide() is called.

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

  delete m_main_window;
  m_main_window = nullptr;

  Dout(dc::notice, "Leaving Application::terminate()");
}

void Application::run(WindowCreateInfo const& main_window_create_info)
{
  DoutEntering(dc::notice|flush_cf, "gui::Application::run(" << main_window_create_info << ")");

  // Do one-time initialization.
  std::call_once(s_main_instance, [this]{ on_main_instance_startup(); });

  // The application has been started, create and show the main window.
  m_main_window = create_window(main_window_create_info);

  //FIXME: what does go here when using vulkan?
  //glfw::makeContextCurrent(*m_main_window);           // Only possible when using GLFW_OPENGL_API or GLFW_OPENGL_ES_API.
#if 0
  //FIXME: is GLEW a vulkan compatible thing?
  if(glewInit() != GLEW_OK)
  {
      throw std::runtime_error("Could not initialize GLEW");
  }
#endif

  // Run the GUI main loop.
  while (!m_main_window->shouldClose())
  {
    // Keep running
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    glfw::pollEvents();
    //FIXME: what does go here when using vulkan?
    //m_main_window->swapBuffers();                     // Only possible when using GLFW_OPENGL_API or GLFW_OPENGL_ES_API.
  }
}

void Application::quit()
{
  DoutEntering(dc::notice, "gui::Application::quit()");
}

} // namespace gui
} // namespace glfw3

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct glfw("GLFW");
NAMESPACE_DEBUG_CHANNELS_END
#endif
