#pragma once

#include <string>
#include <mutex>

class AIEngine;
class LinuxViewerMenuBar;

// This is the GUI implementation that is implemented on top of glfw3.
namespace glfw3 {
namespace gui {

class Window;

class Application
{
 private:
  static std::once_flag s_main_instance;
  Window* m_main_window;

 protected:
  Application(std::string const& application_name);
  virtual ~Application();

 private:
  Window* create_window();

  //void on_window_hide(Gtk::Window* window);

  //---------------------------------------------------------------------------
  // Everything below is GTK independent interface implemented
  // or used by the derived class LinuxViewerApplication.
  //

 protected:
  void terminate();                             // Close the window and cause 'run' to return.

 protected:
  virtual void on_main_instance_startup() = 0;  // This must be called when this is the main instance of the application.
  virtual bool on_gui_idle() = 0;               // This must be called frequently from the main loop of the GUI.
                                                // Return false when there is nothing to be done anymore.
 public:
  virtual void append_menu_entries(LinuxViewerMenuBar* menubar) = 0;    // Derived class must implement this to add menu buttons and get call backs when they are clicked.

 public:
  void run(int argc, char* argv[]);             // Called to run the GUI main loop.
  void quit();                                  // Called to make the GUI main loop terminate (return from run()).
};

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;

namespace menu_keys {
#if 0
  // Make Gtk::Stock IDs available to LinuxViewerApplication::append_menu_entries.
  using namespace Gtk::Stock;
#endif
} // namespace menu_keys
