#pragma once

#include "DispatchLoader.h"
#include "LogicalDevice.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "threadpool/AIThreadPool.h"
#include "threadsafe/aithreadsafe.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "utils/threading/Gate.h"
#include "utils/DequeMemoryResource.h"
#include <boost/intrusive_ptr.hpp>
#include <filesystem>
#include "debug.h"
#ifdef CWDEBUG
#include "debug/DebugUtilsMessenger.h"
#endif

namespace evio {
class EventLoop;
} // namespace evio

namespace resolver {
class Scope;
} // namespace resolver

namespace vulkan {

class InstanceCreateInfo;
class DeviceCreateInfo;

class Application
{
 public:
  // Set up the thread pool for the application.
  static constexpr int default_number_of_threads = 8;                   // Use a thread pool of 8 threads.
  static constexpr int default_reserved_threads = 1;                    // Reserve 1 thread for each priority.

  enum class QueuePriority {
    high,
    medium,
    low
  };

 protected:
  // Create a AIMemoryPagePool object (must be created before thread_pool).
  AIMemoryPagePool m_mpp;

  // Initialize the deque memory resources table.
  utils::DequeMemoryResource::Initialization m_dmri;

  // Create the thread pool.
  AIThreadPool m_thread_pool;

  // And the thread pool queues.
  AIQueueHandle m_high_priority_queue;
  AIQueueHandle m_medium_priority_queue;
  AIQueueHandle m_low_priority_queue;

  // Set up the I/O event loop.
  std::unique_ptr<evio::EventLoop> m_event_loop;
  std::unique_ptr<resolver::Scope> m_resolver_scope;

  // A task that hands out connections to the X server.
  boost::intrusive_ptr<task::SynchronousWindow::xcb_connection_broker_type> m_xcb_connection_broker;

  // Configuration of the main X server connection.
  xcb::ConnectionBrokerKey m_main_display_broker_key;

  // To stop the main thread from exiting until the last xcb connection is closed.
  mutable utils::threading::Gate m_until_terminated;

  // Path with resources.
  std::filesystem::path m_resources_path;

  // All windows.
  using window_list_container_t = std::vector<boost::intrusive_ptr<task::SynchronousWindow>>;
  using window_list_t = aithreadsafe::Wrapper<window_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  window_list_t m_window_list;
  bool m_window_created = false;                        // Set to true the first time a window is created.

  // Loader for vulkan extension functions.
  DispatchLoader m_dispatch_loader;

  // Vulkan instance.
  vk::UniqueInstance m_instance;                        // Per application state. Creating a vk::Instance object initializes
                                                        // the Vulkan library and allows the application to pass information
                                                        // about itself to the implementation. Using vk::UniqueInstance also
                                                        // automatically destroys it.
#ifdef CWDEBUG
  // In order to get the order of destruction correct,
  // this must be defined below m_instance
  // and preferably before m_logical_device_list.
  DebugUtilsMessenger m_debug_utils_messenger;          // Debug message utility extension. Print vulkan layer debug output to dc::vulkan.
#endif

  // All logical devices.
  using logical_device_list_container_t = std::vector<std::unique_ptr<LogicalDevice>>;
  using logical_device_list_t = aithreadsafe::Wrapper<logical_device_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  logical_device_list_t m_logical_device_list;

 private:
  friend class task::SynchronousWindow;
  void add(task::SynchronousWindow* window_task);
  void remove(task::SynchronousWindow* window_task);

  // Counts the number of `boost::intrusive_ptr<Application>` objects.
  // When the last one such object is destructed, the application is terminated.
  mutable std::atomic<int> m_count;

  friend void intrusive_ptr_add_ref(Application const* ptr)
  {
    ptr->m_count.fetch_add(1, std::memory_order_relaxed);
  }

  friend void intrusive_ptr_release(Application const* ptr)
  {
    if (ptr->m_count.fetch_sub(1, std::memory_order_release) == 1)
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      // The last reference was destructed; terminate the application.
      Dout(dc::notice, "Last reference to Application was removed. Terminating application.");
      ptr->m_until_terminated.open();
    }
  }

  // Create and initialize the vulkan instance (m_instance).
  void create_instance(vulkan::InstanceCreateInfo const& instance_create_info);

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(
      vk::Extent2D extent, task::SynchronousWindow::window_cookie_type window_cookie, std::string&& title, task::LogicalDevice const* logical_device);

  friend class task::LogicalDevice;
  int create_device(std::unique_ptr<LogicalDevice>&& logical_device, task::SynchronousWindow* root_window);

  // Accessor.
  vk::Instance vh_instance() const { return *m_instance; }

 public:
  Application();
  ~Application();

 public:
  void initialize(int argc = 0, char** argv = nullptr);

  std::filesystem::path resources_path() const
  {
    return m_resources_path;
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(vk::Extent2D extent, task::SynchronousWindow::window_cookie_type window_cookie, std::string&& title = {})
  {
    // This function is called while creating a vulkan window. The returned value is only used to keep the window alive
    // and/or to pass to create_logical_device in order to find a logical device that supports the surface.
    // Therefore the use of force_synchronous_access is OK: this is only used during initialization.
    return create_root_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(extent, window_cookie, std::move(title), nullptr);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  void create_root_window(vk::Extent2D extent, task::SynchronousWindow::window_cookie_type window_cookie, task::LogicalDevice const& logical_device, std::string&& title = {})
  {
    auto root_window = create_root_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(extent, window_cookie, std::move(title), &logical_device);
    // root_window is immediately destroyed because we don't need it's reference count:
    // the task had already been started.
  }

  boost::intrusive_ptr<task::LogicalDevice> create_logical_device(std::unique_ptr<LogicalDevice>&& logical_device, boost::intrusive_ptr<task::SynchronousWindow const>&& root_window);

  LogicalDevice* get_logical_device(int logical_device_index) const
  {
    logical_device_list_t::crat logical_device_list_r(m_logical_device_list);
    return (*logical_device_list_r)[logical_device_index].get();
  }

  void run();

 protected:
  // Get the default DISPLAY name to use (can be overridden by parse_command_line_parameters).
  virtual std::string default_display_name() const;

  virtual void parse_command_line_parameters(int argc, char* argv[]);

  // Override this function to change the number of worker threads.
  virtual int thread_pool_number_of_worker_threads() const;

  // Override this function to change the size of the thread pool queues.
  virtual int thread_pool_queue_capacity(QueuePriority UNUSED_ARG(priority)) const;

  // Override this function to change the number of reserved threads for each queue (except the last, of course).
  virtual int thread_pool_reserved_threads(QueuePriority UNUSED_ARG(priority)) const;

  // Override this function to change the default ApplicatioInfo values.
  virtual std::string application_name() const;

  // Override this function to change the default application version. The result should be a value returned by vk_utils::encode_version.
  virtual uint32_t application_version() const;

  // Override this function to add Instance layers and/or extensions.
  virtual void prepare_instance_info(vulkan::InstanceCreateInfo& instance_create_info) const { }
};

template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
boost::intrusive_ptr<task::SynchronousWindow const> Application::create_root_window(
    vk::Extent2D extent, task::SynchronousWindow::window_cookie_type window_cookie,
    std::string&& title, task::LogicalDevice const* logical_device_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_root_window<" << libcwd::type_info_of<WINDOW_EVENTS>().demangled_name() <<
      ", " << libcwd::type_info_of<SYNCHRONOUS_WINDOW>().demangled_name() << ">(" << extent << ", " <<
      std::hex << window_cookie << std::dec << ", \"" << title << "\", " << logical_device_task << ")");

  // Call Application::initialize(argc, argv) immediately after constructing the Application.
  //
  // For example:
  //
  //   MyApplication application;
  //   application.initialize(argc, argv);      // <-- this is missing if you assert here.
  //   auto root_window1 = application.create_root_window<MyWindowEvents, MyRenderLoop>({1000, 800}, MyLogicalDevice::root_window_cookie1);
  //
  ASSERT(m_event_loop);

  boost::intrusive_ptr<task::SynchronousWindow> window_task = statefultask::create<SYNCHRONOUS_WINDOW>(this COMMA_CWDEBUG_ONLY(true));
  window_task->create_window_events<WINDOW_EVENTS>(extent);

  // Window initialization.
  if (title.empty())
    title = application_name();
  window_task->set_title(std::move(title));
  window_task->set_window_cookie(window_cookie);
  window_task->set_logical_device_task(logical_device_task);
  // The key passed to set_xcb_connection_broker_and_key MUST be canonicalized!
  m_main_display_broker_key.canonicalize();
  window_task->set_xcb_connection_broker_and_key(m_xcb_connection_broker, &m_main_display_broker_key);

  // Create window and start rendering loop.
  window_task->run();

  // The window is returned in order to pass it to create_logical_device.
  //
  // The pointer should be passed to create_logical_device almost immediately after
  // returning from this function with a std::move.
  return window_task;
}

} // namespace vulkan
