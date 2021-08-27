#pragma once

// We use the GUI implementation on top of glfw3.
#include "ApplicationCreateInfo.h"
#include "DebugUtilsMessengerCreateInfoEXT.h"
#include "Window.h"
#include "GUI_glfw3/gui_Application.h"
#include "GUI_glfw3/gui_WindowCreateInfo.h"
#include "vulkan/InstanceCreateInfo.h"
#include "vulkan/DispatchLoader.h"
#include "vulkan/Pipeline.h"
#include "vulkan/HelloTriangleSwapchain.h"
#include "vulkan/DeviceCreateInfo.h"
#include "vulkan/CommandPoolCreateInfo.h"
#include "statefultask/AIEngine.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "debug.h"
#ifdef CWDEBUG
#include "vulkan/DebugMessenger.h"
#include "vulkan/debug_ostream_operators.h"
#endif

namespace vulkan {
struct DeviceCreateInfo;
} // namespace vulkan

class Application : public gui::Application
{
 protected:
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

  // Loader for vulkan extension functions.
  vulkan::DispatchLoader m_dispatch_loader;

  // Vulkan instance.
  vk::UniqueInstance m_instance;                        // Per application state. Creating a vk::Instance object initializes
                                                        // the Vulkan library and allows the application to pass information
                                                        // about itself to the implementation. Using vk::UniqueInstance also
                                                        // automatically destroys it.
#ifdef CWDEBUG
  // In order to get the order of destruction correct,
  // this must be defined below m_vh_instance,
  // and preferably before m_vulkan_device.
  vulkan::DebugMessenger m_debug_messenger;             // Debug message utility extension. Print vulkan layer debug output to dc::vulkan.
#endif

  // Vulkan graphics.
  vulkan::Device m_vulkan_device;

  std::unique_ptr<vulkan::Pipeline> m_pipeline;

 public:
  // Construct Application from instance_create_info and debug_create_info. Both create_info's are passed as
  // non-const reference because the constructor adds the required glfw extensions to instance_create_info
  // and sets its pNext pointer to point to debug_create_info, using it to print debug output during Instance
  // creation and destruction. While debug_create_info has its pfnUserCallback set to &Application::debugCallback
  // and pUserData to point to this application.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo& instance_create_info
      COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT& debug_create_info));

  // It is perfectly OK to pass an rvalue reference for instance_create_info to the above constructor.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo&& instance_create_info
      COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT& debug_create_info)) :
    Application(application_create_info, instance_create_info COMMA_CWDEBUG_ONLY(debug_create_info)) { }

  // When not passing a instance_create_info, use the default values provided by a plain vulkan::InstanceCreateInfo.
  Application(ApplicationCreateInfo const& application_create_info
      COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT& debug_create_info)) :
    Application(application_create_info, vulkan::InstanceCreateInfo(application_create_info) COMMA_CWDEBUG_ONLY(debug_create_info)) { }

#ifdef CWDEBUG
  // Using an rvalue reference to debug_create_info is OK: this may be a temporary like the default value here.
  // The object that is passed should then not be passed to run() of course: either no debug_create_info should
  // be passed to run(), or another DebugUtilsMessengerCreateInfoEXT object.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo& instance_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info = {}) :
    Application(application_create_info, instance_create_info, debug_create_info) { }

  // It is perfectly OK to pass an rvalue reference for instance_create_info to the above constructor.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo&& instance_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info = {}) :
    Application(application_create_info, instance_create_info, debug_create_info) { }

  // When not passing an instance_create_info, use the default values provided by a plain vulkan::InstanceCreateInfo.
  Application(ApplicationCreateInfo const& application_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info = {}) :
    Application(application_create_info, vulkan::InstanceCreateInfo(application_create_info), debug_create_info) { }
#endif

  ~Application() override
  {
    // Destroy all windows (and hence VkSurface's) before destroying m_vulkan_device.
    terminate();
  }

  // Call this when the application is cleanly terminated and about to go out of scope.
  void join_event_loop() { m_event_loop.join(); }

 private:
  // Keeping track of which of the below functions have been called.
  enum InitializationState {
    istNone,
    istParseCommandLine,
    istMainWindow,
    istDebugMessenger,
    istVulkanDevice,
    istSwapchain,
    istPipeline,
    istCommandBuffers,
    istRun
  };

  InitializationState m_ist_state = istNone;

  void initialization_state(InitializationState const state);

 public:
  // Parse command line options.
  Application& parse_command_line(int argc, char* argv[]);

  // Create application (window, vulkan objects).
  Application& create_main_window(gui::WindowCreateInfo&& main_window_create_info = gui::WindowCreateInfo{});
#ifdef CWDEBUG
  Application& create_debug_messenger(DebugUtilsMessengerCreateInfoEXT&& debug_create_info = DebugUtilsMessengerCreateInfoEXT{});
#endif
  Application& create_vulkan_device(vulkan::DeviceCreateInfo&& device_create_info = vulkan::DeviceCreateInfo{});
  Application& create_swapchain();
  Application& create_pipeline();
  Application& create_command_buffers(vulkan::CommandPoolCreateInfo&& command_pool_create_info = vulkan::CommandPoolCreateInfo{});

  // Called from create_vulkan_device, after creating the vulkan device.
  virtual void init_queue_handles() = 0;

  virtual void create_swapchain_impl() = 0;

  // Start the GUI main loop.
  void run();

#ifdef CWDEBUG
#if 0
  void init(int argc, char* argv[],
      WindowCreateInfo const& main_window_create_info,
      vulkan::DeviceCreateInfo&& device_create_info,
      vulkan::CommandPoolCreateInfo const& command_pool_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info)
  {
    debug_create_info.setup(this);
    init(argc, argv, main_window_create_info, std::move(device_create_info), command_pool_create_info, debug_create_info);
  }

  // Passing no DebugUtilsMessengerCreateInfoEXT will just use the default.
  void init(int argc, char* argv[],
      WindowCreateInfo const& main_window_create_info,
      vulkan::DeviceCreateInfo&& device_create_info,
      vulkan::CommandPoolCreateInfo const& command_pool_create_info)
  {
    init(argc, argv, main_window_create_info, std::move(device_create_info), command_pool_create_info, DebugUtilsMessengerCreateInfoEXT{});
  }
#endif

  static void debug_init();

  void debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData);

  static VkBool32 debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
    void* pUserData)
  {
    Application* self = reinterpret_cast<Application*>(pUserData);
    self->debugCallback(messageSeverity, messageType, pCallbackData);
    return VK_FALSE;
  }
#endif

  bool running() const { return !m_return_from_run; }   // Returns true until quit() was called.
  void quit() override;                                 // Called to make the GUI main loop terminate (return from run()).

 public:
  // Accessors.
  vk::Instance vh_instance() const { return *m_instance; }
  std::shared_ptr<Window> main_window() const;
  Window const* main_window_ptr() const { return static_cast<Window const*>(m_main_window.get()); }

 private:
  void createInstance(vulkan::InstanceCreateInfo const& instance_create_info);
  vk::PipelineLayout createPipelineLayout(vulkan::Device const& device);
  void createPipeline(vulkan::Device const& device_handle, vulkan::HelloTriangleSwapchain const* swapchain_ptr, vk::PipelineLayout vh_pipeline_layout);

 private:
  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
