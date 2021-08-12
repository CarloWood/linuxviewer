#pragma once

#include "Application.h"

class HelloTriangleVulkanApplication : public Application
{
 public:
  using Application::Application;

 private:
  vk::Queue m_graphics_queue;
  vk::Queue m_present_queue;

 private:
  // Menu button events.
  void on_menu_File_QUIT();

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Implementation of append_menu_entries: add File->QUIT with callback on_menu_File_QUIT.
  void append_menu_entries(LinuxViewerMenuBar* menubar) override;

 protected:
  void init_queue_handles() override
  {
    vulkan::QueueRequestIndex request_index(0);
    m_graphics_queue = m_vulkan_device2.get_queue_handle(request_index++, 0);
    m_present_queue = m_vulkan_device2.get_queue_handle(request_index++, 0);
  }
};
