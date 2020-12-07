#pragma once

#include <gtkmm.h>

class LinuxViewerMenuBar;
class LinuxViewerApplication;

class LinuxViewerWindow : public Gtk::Window
{
 private:
  LinuxViewerApplication* m_application;        // The appliction that was passed to the constructor.

 public:
  LinuxViewerWindow(LinuxViewerApplication* application);
  ~LinuxViewerWindow() override;

  void append_menu_entries(LinuxViewerMenuBar* menubar);

 protected:
  friend class LinuxViewerMenuBar;       // Calls append_menu_entries on the two objects returned by the following accessors.
  LinuxViewerApplication& application() const { return *m_application; }

  // Child widgets.
  Gtk::Grid m_grid;
  LinuxViewerMenuBar* m_menubar;
};
