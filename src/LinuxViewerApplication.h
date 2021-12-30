#pragma once

#include "vulkan/Application.h"
#include "debug.h"

class LinuxViewerApplication : public vulkan::Application
{
  using vulkan::Application::Application;

 private:
  int thread_pool_number_of_worker_threads() const override
  {
    // Lets use 4 worker threads in the thread pool.
    return 4;
  }

 public:
  std::string application_name() const override
  {
    return "LinuxViewerApplication";
  }

  void quit()
  {
    DoutEntering(dc::notice, "LinuxViewerApplication::quit()");
    //FIXME: close all windows - that should terminate the application.
  }

#if 0 // FIXME: no menu support!
 private:
  // Menu button events.
  void on_menu_File_QUIT();

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Implementation of append_menu_entries: add File->QUIT with callback on_menu_File_QUIT.
  void append_menu_entries(LinuxViewerMenuBar* menubar) override;
#endif
};
