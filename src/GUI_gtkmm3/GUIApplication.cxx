#include "sys.h"
#include "GUIApplication.h"
#include "GUIWindow.h"
#include "GUIMenuBar.h"
#include "helloworld-task/HelloWorld.h"
#include "statefultask/AIEngine.h"
#include <gtkmm.h>
#include "debug.h"

namespace gtkmm3 {

//static
Glib::RefPtr<GUIApplication> GUIApplication::create(AIEngine& main_engine)
{
  return Glib::RefPtr<GUIApplication>(new GUIApplication(main_engine));
}

GUIApplication::GUIApplication(AIEngine& gtkmm_idle_engine) : m_gtkmm_idle_engine(gtkmm_idle_engine)
{
  DoutEntering(dc::notice, "GUIApplication::GUIApplication(gtkmm_idle_engine)");
  Glib::set_application_name("GUI");
  Glib::signal_idle().connect(sigc::mem_fun(*this, &GUIApplication::on_idle));
}

GUIApplication::~GUIApplication()
{
  Dout(dc::notice, "Calling GUIApplication::~GUIApplication()");
}

void GUIApplication::on_startup()
{
  DoutEntering(dc::notice, "GUIApplication::on_startup()");

  auto task = task::create<task::HelloWorld>();
  task->initialize(42);
  task->run(&m_gtkmm_idle_engine, [=](bool CWDEBUG_ONLY(success)){
      Dout(dc::notice, "Inside the call-back (" <<
          (success ? "success" : "failure") << ").");
      task->run();
  });

  // Call the base class's implementation.
  Gtk::Application::on_startup();
}

void GUIApplication::on_activate()
{
  DoutEntering(dc::notice, "GUIViewer::on_activate()");

  // The application has been started, create and show the main window.
  m_main_window = create_window();
}

GUIWindow* GUIApplication::create_window()
{
  GUIWindow* main_window = new GUIWindow(this);

  // Make sure that the application runs as long this window is still open.
  add_window(*main_window);
  std::vector<Gtk::Window*> windows = get_windows();
  Dout(dc::notice, "The application has " << windows.size() << " windows.");
  ASSERT(G_IS_OBJECT(main_window->gobj()));

  // Delete the window when it is hidden.
  main_window->signal_hide().connect(sigc::bind<Gtk::Window*>(sigc::mem_fun(*this, &GUIApplication::on_window_hide), main_window));

  main_window->show_all();
  return main_window;
}

void GUIApplication::on_window_hide(Gtk::Window* window)
{
  DoutEntering(dc::notice, "GUIApplication::on_window_hide(" << window << ")");
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

  Dout(dc::notice, "Leaving GUIApplication::on_window_hide()");
}

void GUIApplication::append_menu_entries(GUIMenuBar* menubar)
{
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &GUIApplication::on_menu_##top##_##entry)

  ADD(File, QUIT);
}

void GUIApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "GUIApplication::on_menu_File_QUIT()");

  quit(); // Not really necessary, when Gtk::Widget::hide() is called.

  // Gio::Application::quit() will make Gio::Application::run() return,
  // but it's a crude way of ending the program. The window is not removed
  // from the application. Neither the window's nor the application's
  // destructors will be called, because there will be remaining reference
  // counts in both of them. If we want the destructors to be called, we  
  // must remove the window from the application. One way of doing this 
  // is to hide the window.
  std::vector<Gtk::Window*> windows = get_windows();
  Dout(dc::notice, "The application has " << windows.size() << " windows.");
  for (auto window : windows)
    window->hide();

  Dout(dc::notice, "Leaving GUIApplication::on_menu_File_QUIT()");
}

bool GUIApplication::on_idle()
{
  m_gtkmm_idle_engine.mainloop();
  return true;
}

} // namespace gtkmm3
