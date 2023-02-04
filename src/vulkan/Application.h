#ifndef VULKAN_APPLICATION_H
#define VULKAN_APPLICATION_H

#include "DispatchLoader.h"
#include "LogicalDevice.h"
#include "Directories.h"
#include "Concepts.h"
#include "GraphicsSettings.h"
#include "shader_builder/VertexAttribute.h"
#include "shader_builder/ShaderInfos.h"
#include "descriptor/SetKeyContext.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/Broker.h"
#include "statefultask/RunningTasksTracker.h"
#include "threadpool/AIThreadPool.h"
#include "threadsafe/aithreadsafe.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "utils/threading/Gate.h"
#include "utils/DequeMemoryResource.h"
#include "utils/Vector.h"
#include <boost/intrusive_ptr.hpp>
#include <filesystem>
#include <deque>
#ifdef CWDEBUG
#include "debug/DebugUtilsMessenger.h"
#include "cwds/debug_ostream_operators.h"
#endif
#include "debug.h"

namespace evio {
class EventLoop;
} // namespace evio

namespace resolver {
class Scope;
} // namespace resolver

namespace vulkan {

namespace task {
class PipelineFactory;
class PipelineCache;
class SynchronousWindow;
} // namespace task

class InstanceCreateInfo;
class DeviceCreateInfo;
class ImGui;

class Application
{
 public:
  // The same types as used in SynchronousWindow.
  using request_cookie_type = QueueRequest::cookies_type;
  using xcb_connection_broker_type = ::task::Broker<::task::XcbConnection>;
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

  // Accessed by tasks that depend on objects of this class (or derived classes).
  statefultask::RunningTasksTracker m_dependent_tasks;                    // Tasks that should be aborted before the application is destructed.

  static Application& instance() { return *s_instance; }

 protected:
  // Create a AIMemoryPagePool object (must be created before thread_pool).
  AIMemoryPagePool m_mpp;

  // Initialize the deque memory resources table.
  utils::DequeMemoryResource::Initialization m_dmri;

  // Application-wide used node memory resource objects for use with utils::DequeAllocator's with elements of size 512 or less.
  // That is, the assumption is that deque *always* allocates chunks of 512 bytes in that case (it does at the moment of writing).
  // Once it doesn't, the program will assert in utils/NodeMemoryResource.h with: Assertion `block_size <= stored_block_size' failed.
  utils::NodeMemoryResource m_deque512_nmr{m_mpp.instance(), 512};  // _GLIBCXX_DEQUE_BUF_SIZE

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
  mutable utils::threading::Gate m_until_windows_terminated;            // This Gate is opened when the last window is destructed.

  // All windows.
  using window_list_container_t = std::vector<boost::intrusive_ptr<task::SynchronousWindow>>;
  using window_list_t = aithreadsafe::Wrapper<window_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  window_list_t m_window_list;
  bool m_window_created = false;                        // Set to true the first time a window is created.

  // Loader for vulkan extension functions.
  DispatchLoader m_dispatch_loader;

  // Vulkan instance.
  vk::UniqueInstance m_instance;                        // Per application state. Creating a vk::UniqueInstance object initializes
                                                        // the Vulkan library and allows the application to pass information
                                                        // about itself to the implementation. Using vk::UniqueInstance also
                                                        // automatically destroys it.
#ifdef CWDEBUG
  // In order to get the order of destruction correct,
  // this must be defined below m_instance
  // and preferably before m_logical_device_list.
  DebugUtilsMessenger m_debug_utils_messenger;          // Debug message utility extension. Print vulkan layer debug output to dc::vulkan.

  // These booleans determine whether or not the associated task is writing debug output.
  bool m_debug_LogicalDevice{false};
  bool m_debug_XcbConnection{false};                    // Also turns on Broker<XcbConnection>.
  bool m_debug_SemaphoreWatcher{false};
  bool m_debug_AsyncSemaphoreWatcher{false};
  bool m_debug_PipelineCache{false};                    // Only prints debug output when also the associated PipelineFactory is turned on.
  bool m_MoveNewPipelines{false};                       // Idem.
  bool m_CopyDataToBuffer{false};
  bool m_CopyDataToImage{false};
  bool m_ImmediateSubmitQueue{false};                   // Only prints debug output when also the associated CopyDataTo* is turned on.
  bool m_CombinedImageSamplerUpdater{false};
#endif

  // All logical devices.
  using logical_device_list_container_t = std::vector<std::unique_ptr<LogicalDevice>>;
  using logical_device_list_t = aithreadsafe::Wrapper<logical_device_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  logical_device_list_t m_logical_device_list;

  Directories m_directories;                            // Manager of directories for data, configuration, resources etc.

 private:
  static Application* s_instance;                       // There can only be one instance of Application. Allow global access.
  vulkan::GraphicsSettings m_graphics_settings;         // Global configuration values for graphics settings.

  // Storage for all shader templates.
  mutable vulkan::shader_builder::ShaderInfos m_shader_infos;    // Mutable because it is updated by register_shaders, which is threadsafe-"const".

  // We have one of these for each pipeline cache filename.
  struct PipelineCacheMerger
  {
    boost::intrusive_ptr<task::PipelineCache> merged_pipeline_cache;    // The pipeline cache to merge into and finally write to disk.
    std::vector<task::SynchronousWindow const*> window_list;            // A list of all windows that use the same pipeline cache filename.
  };

  using pipeline_factory_list_container_t = std::map<std::u8string, PipelineCacheMerger>;
  using pipeline_factory_list_t = aithreadsafe::Wrapper<pipeline_factory_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  pipeline_factory_list_t m_pipeline_factory_list;

#ifdef TRACY_ENABLE
  task::SynchronousWindow* m_tracy_window{};
#endif

 private:
  friend class task::SynchronousWindow;
  void add(task::SynchronousWindow* window_task);
  void remove(task::SynchronousWindow* window_task);
  void copy_graphics_settings_to(vulkan::GraphicsSettingsPOD* target, LogicalDevice const* logical_device) const;
  void synchronize_graphics_settings() const;           // Call this after changing m_graphics_settings, to synchronize it with the SynchronousWindow objects.

  // Counts the number of `boost::intrusive_ptr<Application>` objects.
  // When the last one such object is destructed, the application is terminated.
  //
  // Because only task::SynchronousWindow is supposed to have such pointers, this
  // reference count is called m_window_count.
  mutable std::atomic<int> m_window_count;

  friend void intrusive_ptr_add_ref(Application const* ptr)
  {
    ptr->m_window_count.fetch_add(1, std::memory_order_relaxed);
  }

  friend void intrusive_ptr_release(Application const* ptr)
  {
    if (ptr->m_window_count.fetch_sub(1, std::memory_order_release) == 1)
    {
      std::atomic_thread_fence(std::memory_order_acquire);
      // The last reference (window) was destructed; terminate the application.
      Dout(dc::notice, "Last reference to Application was removed. Terminating application.");
      ptr->m_until_windows_terminated.open();
    }
  }

  // Create and initialize the vulkan instance (m_instance).
  void create_instance(vulkan::InstanceCreateInfo const& instance_create_info);

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  boost::intrusive_ptr<task::SynchronousWindow const> create_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Rect2D geometry,
      request_cookie_type request_cookie,
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

#ifdef CWDEBUG
  bool debug_PipelineCache() const { return m_debug_PipelineCache; }
  bool debug_SemaphoreWatcher() const { return m_debug_SemaphoreWatcher; }
  bool debug_AsyncSemaphoreWatcher() const { return m_debug_AsyncSemaphoreWatcher; }
  bool debug_MoveNewPipelines() const { return m_MoveNewPipelines; }
  bool debug_CopyDataToBuffer() const { return m_CopyDataToBuffer; }
  bool debug_CopyDataToImage() const { return m_CopyDataToImage; }
  bool debug_ImmediateSubmitQueue() const { return m_ImmediateSubmitQueue; }
  bool debug_CombinedImageSamplerUpdater() const { return m_CombinedImageSamplerUpdater; }
#endif

  // Closes all windows - resulting in the termination of the application.
  void quit();

  // Accessor for the nmr for deque's.
  utils::NodeMemoryResource& deque512_nmr() { return m_deque512_nmr; }

  AIQueueHandle high_priority_queue() const { return m_high_priority_queue; }
  AIQueueHandle medium_priority_queue() const { return m_medium_priority_queue; }
  AIQueueHandle low_priority_queue() const { return m_low_priority_queue; }

  std::filesystem::path path_of(Directory directory) const
  {
    return m_directories.path_of(directory);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Extent2D extent,
      request_cookie_type request_cookie,
      std::u8string&& title = {})
  {
    // This function is called while creating a vulkan window. The returned value is only used to keep the window alive
    // and/or to pass to create_logical_device in order to find a logical device that supports the surface.
    // Therefore the use of force_synchronous_access is OK: this is only used during initialization.
    return create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::move(window_constructor_args), extent, request_cookie, std::move(title), nullptr);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  boost::intrusive_ptr<task::SynchronousWindow const> create_root_window(
      vk::Extent2D extent,
      request_cookie_type request_cookie,
      std::u8string&& title = {})
  {
    // This function is called while creating a vulkan window. The returned value is only used to keep the window alive
    // and/or to pass to create_logical_device in order to find a logical device that supports the surface.
    // Therefore the use of force_synchronous_access is OK: this is only used during initialization.
    return create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::make_tuple(),
        { default_root_window_position, extent }, request_cookie, std::move(title), nullptr);
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW, typename... SYNCHRONOUS_WINDOW_ARGS>
  void create_root_window(
      std::tuple<SYNCHRONOUS_WINDOW_ARGS...>&& window_constructor_args,
      vk::Extent2D extent,
      request_cookie_type request_cookie,
      task::LogicalDevice const& logical_device,
      std::u8string&& title = {})
  {
    auto root_window = create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::move(window_constructor_args), extent, request_cookie, std::move(title), &logical_device);
    // root_window is immediately destroyed because we don't need its reference count:
    // the task has already been started.
  }

  template<ConceptWindowEvents WINDOW_EVENTS, ConceptSynchronousWindow SYNCHRONOUS_WINDOW>
  void create_root_window(
      vk::Extent2D extent,
      request_cookie_type request_cookie,
      task::LogicalDevice const& logical_device,
      std::u8string&& title = {})
  {
    auto root_window = create_window<WINDOW_EVENTS, SYNCHRONOUS_WINDOW>(std::make_tuple(),
        { default_root_window_position, extent}, request_cookie, std::move(title), &logical_device);
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

 public:
  // Called by user functions (e.g. Window::register_shaders).
  // Marked const because it is thread-safe; it isn't really const.
  std::vector<vulkan::shader_builder::ShaderIndex> register_shaders(std::vector<vulkan::shader_builder::ShaderInfo>&& new_shader_info_list) const;

  // Return a reference to the ShaderInfo that corresponds to shader_index, as added by a call to register_shaders.
  vulkan::shader_builder::ShaderInfo const& get_shader_info(vulkan::shader_builder::ShaderIndex shader_index) const;

  // Called by SynchronousWindow::create_pipeline_factory.
  void run_pipeline_factory(boost::intrusive_ptr<task::PipelineFactory> const& factory, task::SynchronousWindow* window, PipelineFactoryIndex index);
  // Called by SynchronousWindow::pipeline_factory_done.
  void pipeline_factory_done(task::SynchronousWindow const* window, boost::intrusive_ptr<task::PipelineCache>&& pipeline_cache);

  // Called by SynchronousWindow::consume_input_events.
  void on_mouse_enter(task::SynchronousWindow* window, int x, int y, bool entered);

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

  // Override this function to add Instance layers and/or extensions.
  virtual void prepare_instance_info(vulkan::InstanceCreateInfo& instance_create_info) const { }

 public:
  // Override this function to change the default ApplicatioInfo values.
  virtual std::u8string application_name() const;

  // Override this function to change the default application version. The result should be a value returned by vk_utils::encode_version.
  virtual uint32_t application_version() const;
};

} // namespace vulkan
#endif // VULKAN_APPLICATION_H
