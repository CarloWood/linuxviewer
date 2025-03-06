#include "sys.h"
#include "SynchronousWindow.h"
#include "FrameResourcesData.h"
#include "PersistentAsyncTask.h"
#include "shader_builder/ShaderIndex.h"
#include "pipeline/PipelineCache.h"
#include "infos/ApplicationInfo.h"
#include "infos/InstanceCreateInfo.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "utils/malloc_size.h"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <cctype>
#ifdef CWDEBUG
#include "debug/DebugUtilsMessengerCreateInfoEXT.h"
#include "debug/vulkan_print_on.h"
#include "utils/debug_ostream_operators.h"
#endif
#include "debug.h"

namespace vulkan {

//static
Application* Application::s_instance;

// Construct the base class of the Application object.
//
// Because this is a base class, virtual functions can't be used in the constructor.
// Therefore initialization happens after construction.
Application::Application() : m_dmri(m_mpp.instance()), m_thread_pool(1), m_dependent_tasks(utils::max_malloc_size(4096))
{
  DoutEntering(dc::vulkan, "vulkan::Application::Application()");
  s_instance = this;
}

// This instantiates the destructor of our std::unique_ptr's.
// Because it is here instead of the header we can use forward declarations for EventLoop and DnsResolver.
Application::~Application()
{
  DoutEntering(dc::vulkan, "vulkan::Application::~Application()");
  // Revoke global access.
  s_instance = nullptr;
}

//virtual
std::string Application::default_display_name() const
{
  DoutEntering(dc::vulkan, "vulkan::Application::default_display_name()");
  return ":0";
}

//virtual
void Application::parse_command_line_parameters(int argc, char* argv[])
{
  DoutEntering(dc::vulkan, "vulkan::Application::parse_command_line_parameters(" << argc << ", " << NAMESPACE_DEBUG::print_argv(argv) << ")");
}

//virtual
std::u8string Application::application_name() const
{
  return reinterpret_cast<char8_t const*>(vk_defaults::ApplicationInfo::default_application_name);
}

//virtual
uint32_t Application::application_version() const
{
  return vk_defaults::ApplicationInfo::default_application_version;
}

// Finish initialization of a default constructed Application.
void Application::initialize(int argc, char** argv)
{
  DoutEntering(dc::vulkan, "vulkan::Application::initialize(" << argc << ", ...)");

  // Only call initialize once. Calling it twice leads to a nasty dead-lock that was hard to debug ;).
  ASSERT(!m_event_loop);

  try
  {
    // Set a default display name.
    m_main_display_broker_key.set_display_name(default_display_name());
  }
  catch (AIAlert::Error const& error)
  {
    // It is not a problem when the default_display_name() is empty (that is the same as not
    // calling set_display_name at all, here). So just print a warning and continue.
    Dout(dc::warning, "\e[31m" << error << ", caught in vulkan/Application.cxx\e[0m");
  }

  try
  {
    // Parse command line parameters before doing any initialization, so the command line arguments can influence the initialization too.

    // Allow the user to override stuff.
    if (argc > 0)
      parse_command_line_parameters(argc, argv);

    // Initialize base directories.
    m_directories.initialize(application_name(), argv[0]);

    // Initialize the thread pool.
    m_thread_pool.change_number_of_threads_to(thread_pool_number_of_worker_threads());
    Debug(m_thread_pool.set_color_functions([](int color){
      static std::array<std::string, 32> color_on_escape_codes = {
        "\e[38;5;1m",
        "\e[38;5;190m",
        "\e[38;5;2m",
        "\e[38;5;33m",
        "\e[38;5;206m",
        "\e[38;5;3m",
        "\e[38;5;130m",
        "\e[38;5;15m",
        "\e[38;5;84m",
        "\e[38;5;63m",
        "\e[38;5;200m",
        "\e[38;5;202m",
        "\e[38;5;9m",
        "\e[38;5;8m",
        "\e[38;5;160m",
        "\e[38;5;222m",
        "\e[38;5;44m",
        "\e[38;5;5m",
        "\e[38;5;210m",
        "\e[38;5;28m",
        "\e[38;5;11m",
        "\e[38;5;225m",
        "\e[38;5;124m",
        "\e[38;5;10m",
        "\e[38;5;4m",
        "\e[38;5;140m",
        "\e[38;5;136m",
        "\e[38;5;250m",
        "\e[38;5;6m",
        "\e[38;5;27m",
        "\e[38;5;123m",
        "\e[38;5;220m"
      };
      ASSERT(0 <= color && color < color_on_escape_codes.size());
      return color_on_escape_codes[color];
    }));

    // Initialize the first thread pool queue.
    m_high_priority_queue   = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::high), thread_pool_reserved_threads(QueuePriority::high));
  }
  catch (AIAlert::Error const& error)
  {
    // If an exception is thrown before we have at least one thread pool queue, then that
    // exception is never caught by main(), because leaving initialize() before that point
    // will cause the application to terminate in AIThreadPool::Worker::tmain(int) with
    // FATAL         : No thread pool queue found after 100 ms. [...]
    // while the main thread is blocked in the destructor of AIThreadPool, waiting to join
    // with the one thread that was already created.
    Dout(dc::warning, "\e[31m" << error << ", caught in vulkan/Application.cxx\e[0m");
    return;
  }

  // Initialize the remaining thread pool queues.
  m_medium_priority_queue = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::medium), thread_pool_reserved_threads(QueuePriority::medium));
  m_low_priority_queue    = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::low));

  // Set up the I/O event loop.
  m_event_loop = std::make_unique<evio::EventLoop>(m_low_priority_queue COMMA_CWDEBUG_ONLY("\e[36m", "\e[0m"));
  m_resolver_scope = std::make_unique<resolver::Scope>(m_low_priority_queue, false);

  // Start the connection broker.
  m_xcb_connection_broker = statefultask::create<xcb_connection_broker_type>(CWDEBUG_ONLY(m_debug_XcbConnection));
  m_xcb_connection_broker->run(m_low_priority_queue);           // Note: the broker never finishes, until abort() is called on it.

  ApplicationInfo application_info;
  application_info.set_application_name(application_name());
  application_info.set_application_version(application_version());
  InstanceCreateInfo instance_create_info(application_info.read_access());

#ifdef CWDEBUG
  // Turn on required debug channels.
  vk_defaults::debug_init();

  // Set debug call back function to call the static DebugUtilsMessenger::debugCallback(nullptr);
  DebugUtilsMessengerCreateInfoEXT debug_create_info(&DebugUtilsMessenger::debugCallback, nullptr);
  // Add extension debug_create_info to use during Instance creation and destruction.
  VkDebugUtilsMessengerCreateInfoEXT& debug_create_info_ref = static_cast<VkDebugUtilsMessengerCreateInfoEXT&>(debug_create_info);
  instance_create_info.setPNext(&debug_create_info_ref);
#endif

  prepare_instance_info(instance_create_info);
  create_instance(instance_create_info);

#ifdef CWDEBUG
  m_debug_utils_messenger.prepare(*m_instance, debug_create_info);
#endif
}

void Application::quit()
{
  DoutEntering(dc::vulkan, "vulkan::Application::quit()");
  window_list_t::wat window_list_w(m_window_list);
  for (auto&& window : *window_list_w)
    window->close();
}

boost::intrusive_ptr<task::LogicalDevice> Application::create_logical_device(
    std::unique_ptr<LogicalDevice>&& logical_device, boost::intrusive_ptr<task::SynchronousWindow const>&& root_window)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_logical_device(" << (void*)logical_device.get() << ", " << (void const*)root_window.get() << ")");

  auto logical_device_task = statefultask::create<task::LogicalDevice>(this COMMA_CWDEBUG_ONLY(m_debug_LogicalDevice));
  logical_device_task->set_logical_device(std::move(logical_device));
  logical_device_task->set_root_window(std::move(root_window));

  logical_device_task->run(m_high_priority_queue);

  return logical_device_task;
}

int Application::create_device(std::unique_ptr<LogicalDevice>&& logical_device, task::SynchronousWindow* root_window)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_device(" << (void*)logical_device.get() << ", [" << root_window << "])");

  logical_device->prepare(*m_instance, m_dispatch_loader, root_window);
  Dout(dc::vulkan, "Created LogicalDevice " << *logical_device);

  int logical_device_index;
  {
    logical_device_list_t::wat logical_device_list_w(m_logical_device_list);
    logical_device_index = logical_device_list_w->size();
    logical_device_list_w->emplace_back(std::move(logical_device));
  }

  root_window->set_logical_device_index(logical_device_index);

  return logical_device_index;
}

void Application::synchronize_graphics_settings() const
{
  DoutEntering(dc::vulkan, "vulkan::Application::synchronize_graphics_settings()");
  window_list_t::crat window_list_r(m_window_list);
  for (auto&& window : *window_list_r)
    window->add_synchronous_task([](task::SynchronousWindow* window_task){ window_task->copy_graphics_settings(); });
}

// This function is executed synchronous with the window that owns target, from SynchronousWindow::copy_graphics_settings.
void Application::copy_graphics_settings_to(vulkan::GraphicsSettingsPOD* target, LogicalDevice const* logical_device) const
{
  DoutEntering(dc::vulkan, "vulkan::Application::copy_graphics_settings_to(" << target << ")");
  *target = GraphicsSettings::crat(m_graphics_settings)->pod();
  Dout(dc::warning(logical_device->max_sampler_anisotropy() < target->maxAnisotropy),
      "Attempting to set a max. anisotropy larger than the logical device (\"" << logical_device->debug_name() << "\") supports.");
  target->maxAnisotropy = std::clamp(target->maxAnisotropy, 1.f, logical_device->max_sampler_anisotropy());
}

void Application::add(task::SynchronousWindow* window_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::add(" << window_task << ")");
  window_list_t::wat window_list_w(m_window_list);
  if (m_window_created && window_list_w->empty())
  {
    // This is not allowed because the program is already terminating, or could be;
    // allowing this would introduce race conditions.
    THROW_ALERT("Trying to add a new window after the last window was closed.");
  }
  m_window_created = true;
  window_list_w->emplace_back(window_task);
}

void Application::remove(task::SynchronousWindow* window_task)
{
  DoutEntering(dc::vulkan, "vulkan::Application::remove(" << window_task << ")");
  window_list_t::wat window_list_w(m_window_list);
  window_list_w->erase(
      std::remove_if(window_list_w->begin(), window_list_w->end(), [window_task](auto element){ return element.get() == window_task; }),
      window_list_w->end());
}

void Application::create_instance(InstanceCreateInfo const& instance_create_info)
{
  DoutEntering(dc::vulkan, "vulkan::Application::create_instance(" << instance_create_info.read_access() << ")");

  // Check that all required layers are available.
  instance_create_info.check_instance_layers_availability();

  // Check that all required extensions are available.
  instance_create_info.check_instance_extensions_availability();

  Dout(dc::vulkan|continued_cf|flush_cf, "Calling vk::createInstanceUnique()... ");
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
#endif
  m_instance = vk::createInstanceUnique(instance_create_info.read_access());
#ifdef CWDEBUG
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
#endif
  Dout(dc::finish, "done (" << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << " ms)");
  // Mandatory call after creating the vulkan instance.
  m_dispatch_loader.load(*m_instance);
}

// Run the application.
// This function does not return until the program terminated.
void Application::run()
{
  // The main thread goes to sleep for the entirety of the application.
  m_until_windows_terminated.wait();

  Dout(dc::notice, "======= Program terminating ======");

  // Terminate all running PersistentAsyncTask's.
  task::PersistentAsyncTask::terminate_and_wait();

  // Wait till all logical devices are idle.
  {
    logical_device_list_t::rat logical_device_list_r(m_logical_device_list);
    for (auto& device : *logical_device_list_r)
      device->wait_idle();
  }

  // Stop the broker tasks.
  m_xcb_connection_broker->terminate();

  // Abort dependent tasks.
  m_dependent_tasks.abort_all();

  // Application terminated cleanly.
  m_event_loop->join();
}

// This member function isn't really const; it is thread-safe.
std::vector<shader_builder::ShaderIndex> Application::register_shaders(std::vector<shader_builder::ShaderInfo>&& new_shader_info_list)
  /*threadsafe-*/const
{
  DoutEntering(dc::vulkan, "Application::register_shaders(" << new_shader_info_list << ")");
  using namespace shader_builder;
  // The word 'new' here purely refers to the fact that they are passed to this function as new ShaderInfo objects
  // (so we are allowed to call hash()); if the hash of any of the 'new' ShaderInfo object is already known then we
  // return the old ShaderIndex and discard the ShaderInfo object.
  size_t const number_of_new_shaders = new_shader_info_list.size();
  // Calculate the hash of each new ShaderInfo objects.
  std::vector<std::size_t> hashes(number_of_new_shaders);
  for (size_t i = 0; i < number_of_new_shaders; ++i)
    hashes[i] = new_shader_info_list[i].hash();
  // The result vector with the indices; new indices when the hash didn't exist already, otherwise the old index.
  std::vector<ShaderIndex> new_indices(number_of_new_shaders);
  ShaderIndex first_new_index{0};
  int duplicates = 0;
  {
    shader_builder::ShaderInfos::wat shader_infos_w(m_shader_infos);
    first_new_index += shader_infos_w->deque.size();  // Initialize first_new_index to the next free index.
    ShaderIndex next_index = first_new_index;
    for (size_t i = 0; i < number_of_new_shaders; ++i)
    {
      auto ibp = shader_infos_w->hash_to_index.insert({ hashes[i], next_index });
      // Fill new_indices with a new index or the one that belongs to this hash.
      new_indices[i] = ibp.second ? next_index++ : ibp.first->second;
      duplicates += ibp.second;
    }
  }
#ifdef CWDEBUG
  if (new_shader_info_list.size() != 2 || new_shader_info_list[0].name() != "imgui.vert.glsl")
    m_pipeline_factory_graph.add_shader_template_codes(new_shader_info_list, hashes, new_indices);
#endif
  {
    shader_builder::ShaderInfos::wat shader_infos_w(m_shader_infos);
    // Move the new ShaderInfo objects into m_shader_infos.
    for (size_t i = 0; i < number_of_new_shaders; ++i)
    {
      if (new_indices[i] >= first_new_index)
        shader_infos_w->deque.emplace_back(std::move(new_shader_info_list[i]));
    }
  }

  new_shader_info_list.clear();
  return new_indices;
}

shader_builder::ShaderInfoCache& Application::get_shader_info(shader_builder::ShaderIndex shader_index) const
{
  shader_builder::ShaderInfos::wat shader_infos_w(m_shader_infos);
  // We can return a reference because m_shader_infos_w->deque is a deque, for which
  // references are not invalidated by inserting more elements at the end.
  return shader_infos_w->deque[shader_index];
}

void Application::run_pipeline_factory(boost::intrusive_ptr<task::PipelineFactory> const& factory, task::SynchronousWindow* window, PipelineFactoryIndex index)
{
  // Remember that this factory is running.
  {
    pipeline_factory_list_t::wat pipeline_factory_list_w(m_pipeline_factory_list);
    pipeline_factory_list_w->operator[](window->pipeline_cache_name()).window_list.push_back(window);
  }
#ifdef CWDEBUG
  m_pipeline_factory_graph.add_factory(window, window->get_title(), index);  // window/index uniquely defines a pipeline factory.
#endif
  factory->run(m_medium_priority_queue);
}

void Application::pipeline_factory_done(task::SynchronousWindow const* window, boost::intrusive_ptr<task::PipelineCache>&& pipeline_cache_task)
{
  DoutEntering(dc::vulkan, "Application::pipeline_factory_done(" << window << ", " << static_cast<AIStatefulTask*>(pipeline_cache_task.get()) << ")");

  std::u8string const pipeline_cache_name = window->pipeline_cache_name();
  pipeline_factory_list_container_t::iterator pipeline_cache_merger_iter;
  boost::intrusive_ptr<task::PipelineCache> merged_pipeline_cache;
  // A factory stopped running.
  PipelineCacheMerger* pipeline_cache_merger = nullptr;
  bool last_window = false;
  {
    pipeline_factory_list_t::wat pipeline_factory_list_w(m_pipeline_factory_list);
    pipeline_cache_merger_iter = pipeline_factory_list_w->find(pipeline_cache_name);
    // Paranoia check: it was added before in Application::run_pipeline_factory.
    ASSERT(pipeline_cache_merger_iter != pipeline_factory_list_w->end());
    // Use a reference to the merger instead of the iterator for convenience.
    pipeline_cache_merger = &pipeline_cache_merger_iter->second;
    // Find a matching window pointer in the list, note that for each factory of the same window, the window was added multiple times.
    auto& windows_with_pipeline_cache_name = pipeline_cache_merger->window_list;
    auto iter = std::find(windows_with_pipeline_cache_name.begin(), windows_with_pipeline_cache_name.end(), window);
    // Idem.
    ASSERT(iter != windows_with_pipeline_cache_name.end());
    // Was this the first factory?
    if (!pipeline_cache_merger->merged_pipeline_cache)
    {
      // If this is the first time a factory finished - then just copy its task::PipelineCache to the PipelineCacheMerger.
      pipeline_cache_merger->merged_pipeline_cache = std::move(pipeline_cache_task);
    }
    else
    {
      pipeline_cache_merger->merged_pipeline_cache->have_new_datum(pipeline_cache_task->detach_pipeline_cache());
      pipeline_cache_merger = nullptr;          // pipeline_cache_task is not the merger (this is not the first factory that finishes).
    }
    // Was this the last window?
    windows_with_pipeline_cache_name.erase(iter);
    last_window = windows_with_pipeline_cache_name.empty();
  }
  if (pipeline_cache_merger)
  {
    // pipeline_cache_task was moved to pipeline_cache_merger->merged_pipeline_cache.
    pipeline_cache_merger->merged_pipeline_cache->set_is_merger();
    pipeline_cache_merger->merged_pipeline_cache->signal(task::PipelineCache::factory_finished);
  }
  else
  {
    // This deletes the task.
    pipeline_cache_task->signal(task::PipelineCache::factory_finished);
  }
  // Was this the last window (now with unlocked m_pipeline_factory_list)?
  if (last_window)
  {
    Dout(dc::notice, "That was the last factory with name " << pipeline_cache_name << ".");
    merged_pipeline_cache = std::move(pipeline_cache_merger_iter->second.merged_pipeline_cache);
    // We merged all pipeline caches. Now make it write to disk.
    ASSERT(merged_pipeline_cache->running());
    merged_pipeline_cache->set_producer_finished();
#ifdef CWDEBUG
    // Write the pipeline factory graph.
    std::filesystem::path dot_filename = path_of(Directory::runtime) / (pipeline_cache_name + u8".dot");
    m_pipeline_factory_graph.generate_dot_file(dot_filename);
    Dout(dc::factorygraph, "Wrote pipeline factory graph dot file to " << dot_filename << ".");
#endif
  }
}

void Application::on_mouse_enter(task::SynchronousWindow* window, int x, int y, bool entered)
{
#ifdef TRACY_ENABLE
  if (entered)
  {
    window->set_is_tracy_window(true);
    if (m_tracy_window != window && m_tracy_window)
      m_tracy_window->set_is_tracy_window(false);
    m_tracy_window = window;
  }
#endif
}

} // namespace vulkan
