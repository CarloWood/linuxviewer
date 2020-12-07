#pragma once

#include <gtkmm.h>

class AIEngine;

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {

class GUIWindow;
class GUIMenuBar;

class GUIApplication : public Gtk::Application
{
 private:
  GUIWindow* m_main_window;

 protected:
  GUIApplication(std::string const& application_name);
  ~GUIApplication() override;

 public:
  void append_menu_entries(GUIMenuBar* menubar);

 private:
  void on_startup() override;
  void on_activate() override;

 private:
  GUIWindow* create_window();

  void on_menu_File_QUIT();
  void on_window_hide(Gtk::Window* window);

 protected:
  virtual void on_main_instance_startup() = 0;  // This must be called when this is the main instance of the application (aka, from on_startup()).
  virtual bool on_gui_idle() = 0;               // This must be called frequently from the main loop of the GUI.
                                                // Return false when there is nothing to be done anymore.
};

} // namespace gtkmm3
