#pragma once

#include "LinuxViewerWindow.h"
#include "statefultask/AIEngine.h"
#include <gtkmm.h>

class LinuxViewerApplication : public Gtk::Application
{
 private:
  AIEngine& m_gtkmm_idle_engine;
  LinuxViewerWindow* m_main_window;

 protected:
  LinuxViewerApplication(AIEngine& main_engine);
  ~LinuxViewerApplication() override;

 public:
  static Glib::RefPtr<LinuxViewerApplication> create(AIEngine& main_engine);

  void append_menu_entries(LinuxViewerMenuBar* menubar);

 protected:
  void on_startup() override;
  void on_activate() override;

 private:
  LinuxViewerWindow* create_window();

  void on_menu_File_QUIT();
  void on_window_hide(Gtk::Window* window);
  bool on_idle();
};
