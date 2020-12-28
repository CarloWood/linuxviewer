#pragma once

#include <gtkmm.h>

class AIEngine;
class LinuxViewerMenuBar;

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {

class GUIWindow;

class GUIApplication : public Gtk::Application
{
 private:
  GUIWindow* m_main_window;

 protected:
  GUIApplication(std::string const& application_name);
  ~GUIApplication() override;

 private:
  void on_startup() override;
  void on_activate() override;

 private:
  GUIWindow* create_window();

  void on_window_hide(Gtk::Window* window);

  //---------------------------------------------------------------------------
  // Everything below is GTK independent interface implemented
  // or used by the derived class LinuxViewerApplication.
  //

 protected:
  void terminate();                             // Close the window and cause 'run' to return.

 protected:
  virtual void on_main_instance_startup() = 0;  // This must be called when this is the main instance of the application (aka, from on_startup()).
  virtual bool on_gui_idle() = 0;               // This must be called frequently from the main loop of the GUI.
                                                // Return false when there is nothing to be done anymore.
 public:
  virtual void append_menu_entries(LinuxViewerMenuBar* menubar) = 0;    // Derived class must implement this to add menu buttons and get call backs when they are clicked.
};

} // namespace gtkmm3

namespace menu_keys {
  // Make Gtk::Stock IDs available to LinuxViewerApplication::append_menu_entries.
  using namespace Gtk::Stock;
} // namespace menu_keys
