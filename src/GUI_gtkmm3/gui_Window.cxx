#include "sys.h"
#include "gui_Window.h"
#include "gui_MenuBar.h"

namespace gtkmm3 {
namespace gui {

Window::Window(Application* application) : m_application(application)
{
  DoutEntering(dc::notice, "gui::Window::Window(" << application << ")");

  set_title("GUI");
  set_default_size(500, 800);

  m_menubar = new MenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
}

Window::~Window()
{
  Dout(dc::notice, "Calling Window::~Window()");
}

void Window::append_menu_entries(MenuBar* menubar)
{
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &Window::on_menu_##top##_##entry)

  Gtk::RadioButtonGroup mode;
#define ADD_RADIO(top, entry) \
  menubar->append_radio_menu_entry(mode, {top, entry},   this, &Window::on_menu_##top##_##entry)

//  ADD(File, OPEN);
}

} // namespace gui
} // namespace gtkmm3
