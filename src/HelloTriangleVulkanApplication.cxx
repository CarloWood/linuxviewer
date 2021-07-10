#include "sys.h"
#include "HelloTriangleVulkanApplication.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/AIEngine.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "utils/threading/Gate.h"
#include "utils/debug_ostream_operators.h"

namespace utils { using namespace threading; }

//static
std::unique_ptr<HelloTriangleVulkanApplication> HelloTriangleVulkanApplication::create(AIEngine& gui_idle_engine)
{
  return std::make_unique<HelloTriangleVulkanApplication>(gui_idle_engine);
}

void HelloTriangleVulkanApplication::on_menu_File_QUIT()
{
  DoutEntering(dc::notice, "HelloTriangleVulkanApplication::on_menu_File_QUIT()");
  // Menu entry File --> Quit was called.

  // Close all windows and cause the application to return from run(),
  // which will terminate the application (see application.cxx).
  terminate();
}

void HelloTriangleVulkanApplication::on_main_instance_startup()
{
  DoutEntering(dc::notice, "HelloTriangleVulkanApplication::on_main_instance_startup()");
}

void HelloTriangleVulkanApplication::append_menu_entries(LinuxViewerMenuBar* menubar)
{
  //FIXME - not implemented yet.
}

bool HelloTriangleVulkanApplication::on_gui_idle()
{
  m_gui_idle_engine.mainloop();

  // Returning true means we want to be called again (more work is to be done).
  return false;
}

int main(int argc, char* argv[])
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  // Create a AIMemoryPagePool object (must be created before thread_pool).
  [[maybe_unused]] AIMemoryPagePool mpp;

  // Set up the thread pool for the application.
  int const number_of_threads = 8;                      // Use a thread pool of 8 threads.
  int const max_number_of_threads = 16;                 // This can later dynamically increased to 16 if needed.
  int const queue_capacity = number_of_threads;
  int const reserved_threads = 1;                       // Reserve 1 thread for each priority.
  // Create the thread pool.
  AIThreadPool thread_pool(number_of_threads, max_number_of_threads);
  Debug(thread_pool.set_color_functions([](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; }));
  // And the thread pool queues.
  [[maybe_unused]] AIQueueHandle high_priority_queue   = thread_pool.new_queue(queue_capacity, reserved_threads);
  [[maybe_unused]] AIQueueHandle medium_priority_queue = thread_pool.new_queue(queue_capacity, reserved_threads);
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(queue_capacity);

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue COMMA_CWDEBUG_ONLY("\e[36m", "\e[0m"));
    resolver::Scope resolver_scope(low_priority_queue, false);

    // Task engine to run tasks from the main loop of GTK+.
    AIEngine gui_idle_engine("gui_idle_engine", 2.0);

    // Create main application.
    auto application = HelloTriangleVulkanApplication::create(gui_idle_engine);

    // Run main application.
    application->run(argc, argv);

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error << " caught in HelloTriangleVulkanApplication.cxx");
  }

  Dout(dc::notice, "Leaving main()");
}
