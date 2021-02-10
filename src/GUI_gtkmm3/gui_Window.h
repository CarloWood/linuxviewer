#pragma once

#include <gtkmm.h>

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {
namespace gui {

class MenuBar;
class Application;

class Window : public Gtk::Window
{
 private:
  Application* m_application;           // The appliction that was passed to the constructor.

 public:
  Window(Application* application);
  ~Window() override;

  void append_menu_entries(MenuBar* menubar);

 protected:
  friend class MenuBar;                 // Calls append_menu_entries on the two objects returned by the following accessors.
  Application& application() const { return *m_application; }

  // Child widgets.
  Gtk::Grid m_grid;
  MenuBar* m_menubar;
};

} // namespace gui
} // namespace gtkmm3
