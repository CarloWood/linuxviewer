#pragma once

#include "Application.h"
#include "HelloTriangleVulkanApplicationCreateInfo.h"

class HelloTriangleVulkanApplication : public Application
{
 private:
  int const m_version;

 public:
  HelloTriangleVulkanApplication(HelloTriangleVulkanApplicationCreateInfo const& create_info) : Application(create_info), m_version(create_info.version)
  {
    DoutEntering(dc::notice, "HelloTriangleVulkanApplication::HelloTriangleVulkanApplication(" << create_info << ")");
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
};
