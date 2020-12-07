#include "sys.h"
#include "GUIWindow.h"
#include "GUIMenuBar.h"

namespace gtkmm3 {

GUIWindow::GUIWindow(GUIApplication* application) : m_application(application)
{
  DoutEntering(dc::notice, "GUIWindow::GUIWindow(" << application << ")");

  set_title("GUI");
  set_default_size(500, 800);

  m_menubar = new GUIMenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
}

GUIWindow::~GUIWindow()
{
  Dout(dc::notice, "Calling GUIWindow::~GUIWindow()");
}

void GUIWindow::append_menu_entries(GUIMenuBar* menubar)
{
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &GUIWindow::on_menu_##top##_##entry)

  Gtk::RadioButtonGroup mode;
#define ADD_RADIO(top, entry) \
  menubar->append_radio_menu_entry(mode, {top, entry},   this, &GUIWindow::on_menu_##top##_##entry)

//  ADD(File, OPEN);
}

} // namespace gtkmm3
