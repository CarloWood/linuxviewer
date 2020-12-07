#pragma once

#include <gtkmm.h>

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {

class GUIMenuBar;
class GUIApplication;

class GUIWindow : public Gtk::Window
{
 private:
  GUIApplication* m_application;        // The appliction that was passed to the constructor.

 public:
  GUIWindow(GUIApplication* application);
  ~GUIWindow() override;

  void append_menu_entries(GUIMenuBar* menubar);

 protected:
  friend class GUIMenuBar;              // Calls append_menu_entries on the two objects returned by the following accessors.
  GUIApplication& application() const { return *m_application; }

  // Child widgets.
  Gtk::Grid m_grid;
  GUIMenuBar* m_menubar;
};

} // namespace gtkmm3
