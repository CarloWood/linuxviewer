#pragma once

// This is the GUI implementation that is implemented on top of glfw3.
namespace glfw3 {
namespace gui {

class MenuBar;
class Application;

class Window
{
 private:
  Application* m_application;        // The appliction that was passed to the constructor.

 public:
  Window(Application* application);
  virtual ~Window();

  void append_menu_entries(MenuBar* menubar);

 protected:
  friend class MenuBar;              // Calls append_menu_entries on the two objects returned by the following accessors.
  Application& application() const { return *m_application; }

#if 0
  // Child widgets.
  Gtk::Grid m_grid;
  MenuBar* m_menubar;
#endif
};

} // namespace gui
} // namespace glfw3
