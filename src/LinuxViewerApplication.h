#pragma once

// We use the GUI implementation on top of gtkmm3.
#include "GUI_gtkmm3/GUIApplication.h"

#include "debug.h"
#include <memory>

class LinuxViewerApplication : public gtkmm3::GUIApplication
{
 private:
  AIEngine& m_gui_idle_engine;

 public:
  LinuxViewerApplication(AIEngine& gui_idle_engine) : gtkmm3::GUIApplication("LinuxViewer"), m_gui_idle_engine(gui_idle_engine)
  {
    DoutEntering(dc::notice, "LinuxViewerApplication::LinuxViewerApplication(gui_idle_engine)");
  }

  ~LinuxViewerApplication() override
  {
    Dout(dc::notice, "Calling GUIApplication::~GUIApplication()");
  }

 public:
  static std::unique_ptr<LinuxViewerApplication> create(AIEngine& gui_idle_engine);

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
