#include "sys.h"
#include "LinuxViewerWindow.h"
#include "LinuxViewerMenuBar.h"

LinuxViewerWindow::LinuxViewerWindow(LinuxViewerApplication* application) : m_application(application)
{
  DoutEntering(dc::notice, "LinuxViewerWindow::LinuxViewerWindow(" << application << ")");

  set_title("LinuxViewer");
  set_default_size(500, 800);

  m_menubar = new LinuxViewerMenuBar(this);
  m_grid.attach(*m_menubar, 0, 0);

  add(m_grid);

  show_all_children();
}

LinuxViewerWindow::~LinuxViewerWindow()
{
  Dout(dc::notice, "Calling LinuxViewerWindow::~LinuxViewerWindow()");
}

void LinuxViewerWindow::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  using namespace menu_keys;
  using namespace Gtk::Stock;
#define ADD(top, entry) \
  menubar->append_menu_entry({top, entry},   this, &LinuxViewerWindow::on_menu_##top##_##entry)

  Gtk::RadioButtonGroup mode;
#define ADD_RADIO(top, entry) \
  menubar->append_radio_menu_entry(mode, {top, entry},   this, &LinuxViewerWindow::on_menu_##top##_##entry)

//  ADD(File, OPEN);
}

