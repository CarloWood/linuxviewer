#pragma once

// We use the GUI implementation on top of glfw3.
#include "GUI_glfw3/gui_Application.h"
#include "ApplicationCreateInfo.h"
#include "vulkan/InstanceCreateInfo.h"
#include "DebugUtilsMessengerCreateInfoEXT.h"
#include "vulkan/ExtensionLoader.h"
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
#include "vulkan/DebugMessenger.h"
#include "vulkan/debug_ostream_operators.h"
#endif

class Window;

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

  // Loader for vulkan extension functions.
  vulkan::ExtensionLoader m_extension_loader;

  // Vulkan instance.
  vk::UniqueInstance m_vulkan_instance;                 // Per application state. Creating a vk::Instance object initializes
                                                        // the Vulkan library and allows the application to pass information
                                                        // about itself to the implementation. Using vk::UniqueInstance also
                                                        // automatically destroys it.
#ifdef CWDEBUG
  // In order to get the order of destruction correct,
  // this must be defined below m_vulkan_instance,
  // and preferably before m_vulkan_device.
  vulkan::DebugMessenger m_debug_messenger;             // Debug message utility extension. Print vulkan layer debug output to dc::vulkan.
#endif

  // Vulkan graphics.
  vulkan::HelloTriangleDevice m_vulkan_device;
  std::vector<VkCommandBuffer> m_command_buffers;       // The vulkan command buffers that this application uses.

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
  // Using an rvalue reference to debug_create_info is OK, this may be a temporary like the default value here.
  // The object that is passed should not be passed to run() then of course: either no debug_create_info should
  // be passed to run(), or another DebugUtilsMessengerCreateInfoEXT object.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo& instance_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info = {}) :
    Application(application_create_info, instance_create_info, debug_create_info) { }

  // It is perfectly OK to pass an rvalue reference for instance_create_info to the above constructor.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo&& instance_create_info,
      DebugUtilsMessengerCreateInfoEXT&& debug_create_info = {}) :
    Application(application_create_info, instance_create_info, debug_create_info) { }

  // When not passing a instance_create_info, use the default values provided by a plain vulkan::InstanceCreateInfo.
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

  // Start the GUI main loop.
  void run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT const& debug_create_info));

#ifdef CWDEBUG
  void run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info COMMA_CWDEBUG_ONLY(DebugUtilsMessengerCreateInfoEXT&& debug_create_info))
  {
    debug_create_info.setup(this);
    run(argc, argv, main_window_create_info, debug_create_info);
  }

  // Passing nothing will just use the default DebugUtilsMessengerCreateInfoEXT.
  void run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info)
  {
    run(argc, argv, main_window_create_info, DebugUtilsMessengerCreateInfoEXT{});
  }

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
  vk::Instance vulkan_instance() const { return *m_vulkan_instance; }
  std::shared_ptr<Window> main_window() const;

 private:
  void createInstance(vulkan::InstanceCreateInfo const& instance_create_info);
  vk::PipelineLayout createPipelineLayout(vulkan::HelloTriangleDevice const& device);
  std::unique_ptr<vulkan::Pipeline> createPipeline(VkDevice device_handle, vulkan::HelloTriangleSwapChain const& swap_chain, VkPipelineLayout pipeline_layout_handle);
  void createCommandBuffers(vulkan::HelloTriangleDevice const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain);
  void drawFrame(vulkan::HelloTriangleSwapChain& swap_chain);

 private:
  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
