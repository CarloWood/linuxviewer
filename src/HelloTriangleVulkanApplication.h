#pragma once

// We use the GUI implementation on top of glfw3.
#include "GUI_glfw3/gui_Application.h"
#include "statefultask/AIEngine.h"
#include "debug.h"
#include <memory>

class HelloTriangleVulkanApplication : public gui::Application
{
 private:
  AIEngine m_gui_idle_engine;           // Task engine to run tasks from the gui main loop.

 public:
  HelloTriangleVulkanApplication(float max_duration = 0.0f) : gui::Application("HelloTriangleVulkanApplication"), m_gui_idle_engine("gui_idle_engine", max_duration)
  {
    DoutEntering(dc::notice, "HelloTriangleVulkanApplication::HelloTriangleVulkanApplication(" << max_duration << ")");
  }

  ~HelloTriangleVulkanApplication() override
  {
    Dout(dc::notice, "Calling HelloTriangleVulkanApplication::~HelloTriangleVulkanApplication()");
  }

 private:
  // Menu button events.
  void on_menu_File_QUIT();

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Implementation of append_menu_entries: add File->QUIT with callback on_menu_File_QUIT.
  void append_menu_entries(LinuxViewerMenuBar* menubar) override;

  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
