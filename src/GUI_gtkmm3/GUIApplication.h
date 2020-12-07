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
  AIEngine& m_gtkmm_idle_engine;
  GUIWindow* m_main_window;

 protected:
  GUIApplication(AIEngine& main_engine);
  ~GUIApplication() override;

 public:
  static Glib::RefPtr<GUIApplication> create(AIEngine& main_engine);

  void append_menu_entries(GUIMenuBar* menubar);

 private:
  void on_startup() override;
  void on_activate() override;

 private:
  GUIWindow* create_window();

  void on_menu_File_QUIT();
  void on_window_hide(Gtk::Window* window);
  bool on_idle();
};

} // namespace gtkmm3
