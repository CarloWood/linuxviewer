#include "sys.h"
#include "Application.h"
#include "VulkanWindow.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include <algorithm>
#include "debug.h"
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#endif

namespace vulkan {

// Construct the base class of the Application object.
//
// Because this is a base class, virtual functions can't be used in the constructor.
// Therefore initialization happens after construction.
Application::Application() : m_thread_pool(1)
{
}

// This instantiates the destructor of our std::unique_ptr's. Put here instead of the header
// so that we could use forward declarations for EventLoop and DnsResolver.
Application::~Application()
{
}

// virtual
void Application::parse_command_line_parameter(int argc, char* argv[])
{
  DoutEntering(dc::vulkan, "vulkan::Application::parse_command_line_parameter(" << argc << ", " << debug::print_argv(argv) << ")");
}

// Finish initialization of a default constructed Application.
void Application::initialize(int argc, char** argv)
{
  DoutEntering(dc::vulkan, "vulkan::Application::initialize(" << argc << ", ...)");

  // Only call initialize once. Calling it twice leads to a nasty dead-lock that was hard to debug ;).
  ASSERT(!m_event_loop);

  // Parse command line parameters before doing any initialization, so the command line arguments can influence the initialization too.
  if (argc > 0)
    parse_command_line_parameter(argc, argv);

  // Initialize the thread pool.
  m_thread_pool.change_number_of_threads_to(thread_pool_number_of_worker_threads());
  Debug(m_thread_pool.set_color_functions([](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; }));

  // Initialize the thread pool queues.
  m_high_priority_queue   = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::high), thread_pool_reserved_threads(QueuePriority::high));
  m_medium_priority_queue = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::medium), thread_pool_reserved_threads(QueuePriority::medium));
  m_low_priority_queue    = m_thread_pool.new_queue(thread_pool_queue_capacity(QueuePriority::low));

  // Set up the I/O event loop.
  m_event_loop = std::make_unique<evio::EventLoop>(m_low_priority_queue COMMA_CWDEBUG_ONLY("\e[36m", "\e[0m"));
  m_resolver_scope = std::make_unique<resolver::Scope>(m_low_priority_queue, false);

  // Start the connection broker.
  m_xcb_connection_broker = statefultask::create<task::Broker<task::XcbConnection>>(CWDEBUG_ONLY(false));
  m_xcb_connection_broker->run(m_low_priority_queue);           // Note: the broker never finishes, untill abort() is called on it.
}

void Application::create_main_window(std::unique_ptr<linuxviewer::OS::Window>&& window, std::string&& title, vk::Extent2D extent)
{
  DoutEntering(dc::vulkan, "Application::create_main_window(" << (void*)window.get() << ", \"" << title << "\", " << extent << ")");

  // Call Application::initialize() immediately after constructing the Application.
  //
  // For example:
  //
  //   MyApplication application;
  //   application.initialize(argc, argv);      // <-- this is missing if you assert here.
  //   application.create_main_window("My main window", { 800, 600 }, std::make_unique<MyWindow>());
  //
  ASSERT(m_event_loop);

  auto window_task = statefultask::create<task::VulkanWindow>(this, std::move(window) COMMA_CWDEBUG_ONLY(true));

  // Window initialization.
  window_task->set_title(std::move(title));
  window_task->set_size(extent);
  //broker_key.set_display_name(":0");
  window_task->set_xcb_connection(m_xcb_connection_broker, &m_main_display_broker_key);

  // Create window and start rendering loop.
  window_task->run();
}

void Application::add(task::VulkanWindow* window_task)
{
  DoutEntering(dc::vulkan, "Application::add(" << window_task << ")");
  window_list_t::wat window_list_w(m_window_list);
  window_list_w->emplace_back(window_task);
}

void Application::remove(task::VulkanWindow* window_task)
{
  DoutEntering(dc::vulkan, "Application::remove(" << window_task << ")");
  window_list_t::wat window_list_w(m_window_list);
  window_list_w->erase(
      std::remove_if(window_list_w->begin(), window_list_w->end(), [window_task](auto element){ return element.get() == window_task; }),
      window_list_w->end());
}

// Run the application.
// This function does not return until the program terminated.
void Application::run(int argc, char* argv[])
{
  // The main thread goes to sleep for the entirety of the application.
  m_until_terminated.wait();

  Dout(dc::notice, "======= Program terminated ======");

  // Stop the broker task.
  m_xcb_connection_broker->terminate([](task::XcbConnection* xcb_connection){ xcb_connection->close(); });

  // Application terminated cleanly.
  m_event_loop->join();
}

} // namespace vulkan
