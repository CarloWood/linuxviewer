#include "sys.h"
#include "gui_Window.h"
#include "gui_Application.h"
//#include "gui_MenuBar.h"
#include "debug.h"

namespace glfw3 {
namespace gui {

Window::Window(Application* application, WindowCreateInfoExt const& create_info) :
  glfw::Window(
      create_info.width,
      create_info.height,
      create_info.title ? create_info.title : application->application_name().c_str(),
      create_info.monitor,
      create_info.share),
  m_application(application)
{
  DoutEntering(dc::notice, "Window::Window(" << application << ", " << create_info << ") [" << (void*)this << "] [INCOMPLETE]");

#if 0
  // FIXME: is this still possible now that we do glfw::makeContextCurrent() after returning from this constructor?
  // Maybe this should be delayed till after that.

  m_menubar = new MenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
#endif
}

Window::~Window()
{
  Dout(dc::notice, "Calling Window::~Window() [" << (void*)this << "]");
}

void Window::append_menu_entries(MenuBar* menubar)
{
  DoutEntering(dc::notice, "Window::append_menu_entries(" << (void*)menubar << ") [NOT IMPLEMENTED]");

#if 0
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &Window::on_menu_##top##_##entry)

  Gtk::RadioButtonGroup mode;
#define ADD_RADIO(top, entry) \
  menubar->append_radio_menu_entry(mode, {top, entry},   this, &Window::on_menu_##top##_##entry)
#endif

//  ADD(File, OPEN);
}

} // namespace gui
} // namespace glfw3
