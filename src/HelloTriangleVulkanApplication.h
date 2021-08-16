#pragma once

#include "vulkan/Queue.h"
#include "Application.h"

class HelloTriangleVulkanApplication : public Application
{
 public:
  using Application::Application;

 private:
  vulkan::Queue m_graphics_queue;
  vulkan::Queue m_present_queue;

 private:
  // Menu button events.
  void on_menu_File_QUIT();

 private:
  // Called once, when this is the main instance of the application that is being started.
  void on_main_instance_startup() override;

  // Implementation of append_menu_entries: add File->QUIT with callback on_menu_File_QUIT.
  void append_menu_entries(LinuxViewerMenuBar* menubar) override;

  // Called from create_swap_chain().
  void create_swap_chain_impl() override
  {
    main_window()->createSwapChain(m_vulkan_device, m_graphics_queue, m_present_queue);
  }

 protected:
  void init_queue_handles() override
  {
    vulkan::QueueRequestIndex request_index(0);
    m_graphics_queue = m_vulkan_device.get_queue(request_index++, 0);
    m_present_queue = m_vulkan_device.get_queue(request_index++, 0);
  }
};
