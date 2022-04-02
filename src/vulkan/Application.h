#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "DispatchLoader.h"
#include "LogicalDevice.h"
#include "Directories.h"
#include "Concepts.h"
#include "GraphicsSettings.h"
#include "pipeline/PipelineCache.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "threadpool/AIThreadPool.h"
#include "threadsafe/aithreadsafe.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "utils/threading/Gate.h"
#include "utils/DequeMemoryResource.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>
#include <filesystem>
#include "debug.h"
#ifdef CWDEBUG
#include "debug/DebugUtilsMessenger.h"
#include "cwds/debug_ostream_operators.h"
#endif

namespace task {
class SynchronousWindow;
class PipelineFactory;
} // namespace task

namespace evio {
class EventLoop;
} // namespace evio

namespace resolver {
class Scope;
} // namespace resolver

namespace vulkan {

class InstanceCreateInfo;
class DeviceCreateInfo;
class ImGui;

class Application
{
 public:
  // The same types as used in SynchronousWindow.
  using window_cookie_type = QueueReply::window_cookies_type;
  using xcb_connection_broker_type = task::Broker<task::XcbConnection>;
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<task::PipelineFactory>>;

  // Set up the thread pool for the application.
  static constexpr int default_number_of_threads = 8;                           // Use a thread pool of 8 threads.
  static constexpr int default_reserved_threads = 1;                            // Reserve 1 thread for each priority.
  static constexpr vk::Offset2D default_root_window_position = { 0, 0 };        // Default top-left corner.

  enum class QueuePriority {
    high,
    medium,
    low
  };

  static Application& instance() { return *s_instance; }

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
  boost::intrusive_ptr<xcb_connection_broker_type> m_xcb_connection_broker;

  // Configuration of the main X server connection.
  xcb::ConnectionBrokerKey m_main_display_broker_key;

  // To stop the main thread from exiting until the last xcb connection is closed.
  mutable utils::threading::Gate m_until_terminated;

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

  Directories m_directories;                            // Manager of directories for data, configuration, resources etc.

 private:
  static Application* s_instance;                       // There can only be one instance of Application. Allow global access.
  vulkan::GraphicsSettings m_graphics_settings;         // Global configuration values for graphics settings.

  // We have one of these for each pipeline cache filename.
  struct PipelineCacheMerger
  {
    boost::intrusive_ptr<task::PipelineCache> merged_pipeline_cache;    // The pipeline cache to merge into and finally write to disk.
    std::vector<task::SynchronousWindow const*> window_list;            // A list of all windows that use the same pipeline cache filename.
  };

  using pipeline_factory_list_container_t = std::map<std::u8string, PipelineCacheMerger>;
  using pipeline_factory_list_t = aithreadsafe::Wrapper<pipeline_factory_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_factory_list_t m_pipeline_factory_list;

 private:
  friend class task::SynchronousWindow;
  void add(task::SynchronousWindow* window_task);
  void remove(task::SynchronousWindow* window_task);
  void copy_graphics_settings_to(vulkan::GraphicsSettingsPOD* target, LogicalDevice const* logical_device) const;
  void synchronize_graphics_settings() const;           // Call this after changing m_graphics_settings, to synchronize it with the SynchronousWindow objects.
  void flush_pipeline_caches();                         // Write pipeline caches to disk and free memory resources.

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

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  boost::intrusive_ptr<task::SynchronousWindow const> create_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Rect2D geometry,
      window_cookie_type window_cookie,
      std::u8string&& title,
      task::LogicalDevice const* logical_device,
      task::SynchronousWindow const* parent_window_task = nullptr);

  friend class task::LogicalDevice;
  int create_device(std::unique_ptr<LogicalDevice>&& logical_device, task::SynchronousWindow* root_window);

  // Accessor.
  friend class ImGui;
  vk::Instance vh_instance() const { return *m_instance; }

 public:
  Application();
  ~Application();

 public:
  void initialize(int argc = 0, char** argv = nullptr);

  std::filesystem::path path_of(Directory directory) const
  {
    return m_directories.path_of(directory);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Extent2D extent,
      window_cookie_type window_cookie,
      std::u8string&& title = {})
  {
    // This function is called while creating a vulkan window. The returned value is only used to keep the window alive
    // and/or to pass to create_logical_device in order to find a logical device that supports the surface.
    // Therefore the use of force_synchronous_access is OK: this is only used during initialization.
    return create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::move(window_constructor_args), extent, window_cookie, std::move(title), nullptr);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(
      vk::Extent2D extent,
      window_cookie_type window_cookie,
      std::u8string&& title = {})
  {
    // This function is called while creating a vulkan window. The returned value is only used to keep the window alive
    // and/or to pass to create_logical_device in order to find a logical device that supports the surface.
    // Therefore the use of force_synchronous_access is OK: this is only used during initialization.
    return create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::make_tuple(),
        { default_root_window_position, extent }, window_cookie, std::move(title), nullptr);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  void create_root_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Extent2D extent,
      window_cookie_type window_cookie,
      task::LogicalDevice const& logical_device,
      std::u8string&& title = {})
  {
    auto root_window = create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::move(window_constructor_args), extent, window_cookie, std::move(title), &logical_device);
    // root_window is immediately destroyed because we don't need its reference count:
    // the task has already been started.
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  void create_root_window(
      vk::Extent2D extent,
      window_cookie_type window_cookie,
      task::LogicalDevice const& logical_device,
      std::u8string&& title = {})
  {
    auto root_window = create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::make_tuple(),
        { default_root_window_position, extent}, window_cookie, std::move(title), &logical_device);
    // idem
  }

  boost::intrusive_ptr<task::LogicalDevice> create_logical_device(std::unique_ptr<LogicalDevice>&& logical_device, boost::intrusive_ptr<task::SynchronousWindow const>&& root_window);

  LogicalDevice* get_logical_device(int logical_device_index) const
  {
    logical_device_list_t::crat logical_device_list_r(m_logical_device_list);
    return (*logical_device_list_r)[logical_device_index].get();
  }

  void run();

  void set_max_anisotropy(float max_anisotropy)
  {
    DoutEntering(dc::notice, "Application::set_max_anisotropy(" << max_anisotropy << ")");
    if (GraphicsSettings::wat(m_graphics_settings)->set_max_anisotropy({}, max_anisotropy))
      synchronize_graphics_settings();
  }

  // Called by SynchronousWindow::create_pipeline_factory.
  void run_pipeline_factory(boost::intrusive_ptr<task::PipelineFactory> const& factory, task::SynchronousWindow* window, PipelineFactoryIndex index);
  // Called by SynchronousWindow::pipeline_factory_done.
  void pipeline_factory_done(task::SynchronousWindow const* window, boost::intrusive_ptr<task::PipelineCache>&& pipeline_cache);

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
  virtual std::u8string application_name() const;

  // Override this function to change the default application version. The result should be a value returned by vk_utils::encode_version.
  virtual uint32_t application_version() const;

  // Override this function to add Instance layers and/or extensions.
  virtual void prepare_instance_info(vulkan::InstanceCreateInfo& instance_create_info) const { }
};

} // namespace vulkan

#include "SynchronousWindow.h"
#endif // VULKAN_APPLICATION_H

#ifndef VULKAN_APPLICATION_defs_H
#define VULKAN_APPLICATION_defs_H

namespace vulkan {

template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
boost::intrusive_ptr<task::SynchronousWindow const> Application::create_window(
    std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
    vk::Rect2D geometry, window_cookie_type window_cookie,
    std::u8string&& title, task::LogicalDevice const* logical_device_task,
    task::SynchronousWindow const* parent_window_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_window<" <<
      libcwd::type_info_of<WINDOW_EVENTS>().demangled_name() << ", " <<         // First template parameter (WINDOW_EVENTS).
      libcwd::type_info_of<SYNCHRONOUS_WINDOW>().demangled_name() <<            // Second template parameter (SYNCHRONOUS_WINDOW).
      ((LibcwDoutStream << ... << (std::string(", ") + libcwd::type_info_of<SYNCHRONOUS_WINDOW_ARGS>().demangled_name())), ">(") <<
                                                                                // Tuple template parameters (SYNCHRONOUS_WINDOW_ARGS...)
      window_constructor_args << ", " << geometry << ", " << std::hex << window_cookie << std::dec << ", \"" << title << "\", " << logical_device_task << ", " << parent_window_task << ")");

  // Call Application::initialize(argc, argv) immediately after constructing the Application.
  //
  // For example:
  //
  //   MyApplication application;
  //   application.initialize(argc, argv);      // <-- this is missing if you assert here.
  //   auto root_window1 = application.create_root_window<MyWindowEvents, MyRenderLoop>({1000, 800}, MyLogicalDevice::root_window_cookie1);
  //
  ASSERT(m_event_loop);

  boost::intrusive_ptr<task::SynchronousWindow> window_task =
    statefultask::create_from_tuple<SYNCHRONOUS_WINDOW>(
        std::tuple_cat(
          std::move(window_constructor_args),
          std::make_tuple(this COMMA_CWDEBUG_ONLY(true))
        )
    );
  window_task->create_window_events<WINDOW_EVENTS>(geometry.extent);

  // Window initialization.
  if (title.empty())
    title = application_name();
  window_task->set_title(std::move(title));
  window_task->set_offset(geometry.offset);
  window_task->set_window_cookie(window_cookie);
  window_task->set_logical_device_task(logical_device_task);
  // The key passed to set_xcb_connection_broker_and_key MUST be canonicalized!
  m_main_display_broker_key.canonicalize();
  window_task->set_xcb_connection_broker_and_key(m_xcb_connection_broker, &m_main_display_broker_key);
  // Note that in the case of creating a child window we use the logical device of the parent.
  // However, logical_device_task can be null here because this function might be called before
  // the logical device (or parent window) was created. The SynchronousWindow task takes this
  // into account in state SynchronousWindow_create: where m_logical_device_task is null and
  // m_parent_window_task isn't, it registers with m_parent_window_task->m_logical_device_index_available_event
  // to pick up the correct value of m_logical_device_task.
  window_task->set_parent_window_task(parent_window_task);

  // Create window and start rendering loop.
  window_task->run();

  // The window is returned in order to pass it to create_logical_device.
  //
  // The pointer should be passed to create_logical_device almost immediately after
  // returning from this function with a std::move.
  return window_task;
}

} // namespace vulkan
#endif // VULKAN_APPLICATION_defs_H
