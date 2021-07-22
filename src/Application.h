#pragma once

// We use the GUI implementation on top of glfw3.
#include "GUI_glfw3/gui_Application.h"
#include "ApplicationCreateInfo.h"
#include "vulkan/InstanceCreateInfo.h"
#include "WindowCreateInfo.h"
#include "vulkan/Pipeline.h"
#include "vulkan/HelloTriangleSwapChain.h"
#include "vulkan/HelloTriangleDevice.h"
#include "statefultask/AIEngine.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "debug.h"
#ifdef CWDEBUG
#include "vulkan/debug_ostream_operators.h"
#endif

class Application : public gui::Application
{
 private:
  // Create a AIMemoryPagePool object (must be created before thread_pool).
  AIMemoryPagePool m_mpp;

  // Create the thread pool.
  AIThreadPool m_thread_pool;

  // And the thread pool queues.
  AIQueueHandle m_high_priority_queue;
  AIQueueHandle m_medium_priority_queue;
  AIQueueHandle m_low_priority_queue;

  // Set up the I/O event loop.
  evio::EventLoop m_event_loop;
  resolver::Scope m_resolver_scope;

  // The GUI main loop.
  bool m_return_from_run;                               // False while the (inner) main loop should keep looping.
  AIEngine m_gui_idle_engine;                           // Task engine to run tasks from the gui main loop.

  // Vulkan instance.
  vk::Instance m_vulkan_instance;

  // Vulkan graphics.
  std::vector<VkCommandBuffer> m_command_buffers;       // The vulkan command buffers that this application uses.

 public:
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo const& instance_create_info) :
    gui::Application(application_create_info.pApplicationName),
    m_mpp(application_create_info.block_size, application_create_info.minimum_chunk_size, application_create_info.maximum_chunk_size),
    m_thread_pool(application_create_info.number_of_threads, application_create_info.max_number_of_threads),
    m_high_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
    m_medium_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
    m_low_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity)),
    m_event_loop(m_low_priority_queue COMMA_CWDEBUG_ONLY(application_create_info.event_loop_color, application_create_info.color_off_code)),
    m_resolver_scope(m_low_priority_queue, false),
    m_return_from_run(false),
    m_gui_idle_engine("gui_idle_engine", application_create_info.max_duration),
    m_vulkan_instance{vk::createInstance(instance_create_info)}
  {
    DoutEntering(dc::notice, "Application::Application(" << application_create_info << ", " << instance_create_info << ")");
    m_vulkan_instance = vk::createInstance(instance_create_info);
    Debug(m_thread_pool.set_color_functions(application_create_info.thread_pool_color_function));
  }

  ~Application() override
  {
  }

  // Call this when the application is cleanly terminated and about to go out of scope.
  void join_event_loop() { m_event_loop.join(); }

  // Start the GUI main loop.
  void run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info);

  bool running() const { return !m_return_from_run; }   // Returns true until quit() was called.
  void quit() override;                                 // Called to make the GUI main loop terminate (return from run()).

 private:
  void createPipelineLayout(VkDevice device_handle, VkPipelineLayout* pipelineLayout);
  std::unique_ptr<vulkan::Pipeline> createPipeline(VkDevice device_handle, vulkan::HelloTriangleSwapChain const& swap_chain, VkPipelineLayout pipeline_layout_handle);
  void createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain);
  void drawFrame(vulkan::HelloTriangleSwapChain& swap_chain);

 private:
  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
