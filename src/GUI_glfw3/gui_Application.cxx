#include "sys.h"
#include "gui_Application.h"
#include "gui_Window.h"
#include <vector>
#include "debug.h"

namespace glfw3 {
namespace gui {

std::once_flag Application::s_main_instance;

Application::Application(std::string const& application_name) : m_main_window(nullptr)
{
  DoutEntering(dc::notice, "gui::Application::Application(\"" << application_name << "\") [NOT IMPLEMENTED]");
#if 0
  // Initialize application.
  Glib::set_application_name(application_name);
  Glib::signal_idle().connect(sigc::mem_fun(*this, &Application::on_gui_idle));
#endif
}

Application::~Application()
{
}

Window* Application::create_window()
{
  DoutEntering(dc::notice, "gui::Application::create_window() [NOT IMPLEMENTED]");
  Window* main_window = new Window(this);

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

  Dout(dc::notice, "Leaving Application::terminate()");
}

void Application::run(int argc, char* argv[])
{
  DoutEntering(dc::notice|flush_cf|continued_cf, "gui::Application::run(" << argc << ", ");

  // Do one-time initialization.
  std::call_once(s_main_instance, [this]{ on_main_instance_startup(); });

  char const* prefix = "{";
  for (char** arg = argv; *arg; ++arg)
  {
    Dout(dc::continued|flush_cf, prefix << '"' << *arg << '"');
    prefix = ", ";
  }
  Dout(dc::finish|flush_cf, '}');

  // The application has been started, create and show the main window.
  m_main_window = create_window();
}

void Application::quit()
{
  DoutEntering(dc::notice, "gui::Application::quit()");
}

} // namespace gui
} // namespace glfw3
