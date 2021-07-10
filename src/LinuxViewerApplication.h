#pragma once

#include "Application.h"

class LinuxViewerApplication : public Application
{
 public:
  LinuxViewerApplication(ApplicationCreateInfo const& create_info) : Application(create_info)
  {
    DoutEntering(dc::notice, "LinuxViewerApplication::LinuxViewerApplication(" << create_info << ")");
  }

  ~LinuxViewerApplication() override
  {
    Dout(dc::notice, "Calling LinuxViewerApplication::~LinuxViewerApplication()");
  }

 private:
  // Menu button events.
  void on_menu_File_QUIT();

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Implementation of append_menu_entries: add File->QUIT with callback on_menu_File_QUIT.
  void append_menu_entries(LinuxViewerMenuBar* menubar) override;
};
