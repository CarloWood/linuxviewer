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
#include "vulkan/Device.h"
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
  vulkan::Device m_vulkan_device;
  std::vector<VkCommandBuffer> m_command_buffers;       // The vulkan command buffers that this application uses.

 public:
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo& instance_create_info) :
    gui::Application(application_create_info.pApplicationName),
    m_mpp(application_create_info.block_size, application_create_info.minimum_chunk_size, application_create_info.maximum_chunk_size),
    m_thread_pool(application_create_info.number_of_threads, application_create_info.max_number_of_threads),
    m_high_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
    m_medium_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity, application_create_info.reserved_threads)),
    m_low_priority_queue(m_thread_pool.new_queue(application_create_info.queue_capacity)),
    m_event_loop(m_low_priority_queue COMMA_CWDEBUG_ONLY(application_create_info.event_loop_color, application_create_info.color_off_code)),
    m_resolver_scope(m_low_priority_queue, false),
    m_return_from_run(false),
    m_gui_idle_engine("gui_idle_engine", application_create_info.max_duration)
  {
    // Call this before print the DoutEntering debug output, so we get to see all extensions that are used.
    instance_create_info.add_extensions(glfw::getRequiredInstanceExtensions());

    DoutEntering(dc::notice, "Application::Application(" << application_create_info << ", " << instance_create_info.base() << ")");

    // Set the debug color function (this couldn't be part of the above constructor).
    Debug(m_thread_pool.set_color_functions(application_create_info.thread_pool_color_function));

    // Upon return from this constructor, the application_create_info might be destucted before we get another chance
    // to create the vulkan instance (this is unlikely, but certainly not impossible). And since instance_create_info contains
    // a pointer to it, as well as might itself be destructed, we need to be done with both of them before returning.
    // Hence, the instance has to be created here.
    createInstance(instance_create_info);
  }

  // It is perfectly OK to pass an rvalue to the above constructor.
  Application(ApplicationCreateInfo const& application_create_info, vulkan::InstanceCreateInfo&& instance_create_info) :
    Application(application_create_info, instance_create_info) { }

  // When not passing a instance_create_info, use the default values provided by a plain vulkan::InstanceCreateInfo.
  Application(ApplicationCreateInfo const& application_create_info) :
    Application(application_create_info, vulkan::InstanceCreateInfo(application_create_info)) { }

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
    run(argc, argv, main_window_create_info, debug_create_info);
  }

  // Passing nothing will just use the default DebugUtilsMessengerCreateInfoEXT.
  void run(int argc, char* argv[], WindowCreateInfo const& main_window_create_info)
  {
    run(argc, argv, main_window_create_info, DebugUtilsMessengerCreateInfoEXT(*this));
  }

  VkBool32 debugCallback(
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
    return self->debugCallback(messageSeverity, messageType, pCallbackData);
  }
#endif

  bool running() const { return !m_return_from_run; }   // Returns true until quit() was called.
  void quit() override;                                 // Called to make the GUI main loop terminate (return from run()).

 public:
  // Accessors.
  vk::Instance vulkan_instance() const { return *m_vulkan_instance; }

 private:
  void createInstance(vulkan::InstanceCreateInfo const& instance_create_info);
  std::shared_ptr<Window> main_window() const;
  void setupDebugMessenger();
  vk::PipelineLayout createPipelineLayout(vulkan::Device const& device);
  std::unique_ptr<vulkan::Pipeline> createPipeline(VkDevice device_handle, vulkan::HelloTriangleSwapChain const& swap_chain, VkPipelineLayout pipeline_layout_handle);
  void createCommandBuffers(vulkan::Device const& device, vulkan::Pipeline* pipeline, vulkan::HelloTriangleSwapChain const& swap_chain);
  void drawFrame(vulkan::HelloTriangleSwapChain& swap_chain);

 private:
  // Called from the main loop of the GUI.
  bool on_gui_idle() override;
};
