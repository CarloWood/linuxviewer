#include "sys.h"
#include "LinuxViewerApplication.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "statefultask/AIEngine.h"
#include "evio/EventLoop.h"
#include "utils/debug_ostream_operators.h"

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
  // And the thread pool queues.
  [[maybe_unused]] AIQueueHandle high_priority_queue   = thread_pool.new_queue(queue_capacity, reserved_threads);
  [[maybe_unused]] AIQueueHandle medium_priority_queue = thread_pool.new_queue(queue_capacity, reserved_threads);
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(queue_capacity);

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue);

    // Task engine to run tasks from the main loop of GTK+.
    AIEngine gui_idle_engine("gui_idle_engine", 2.0);

    // Create main application.
    auto application = LinuxViewerApplication::create(gui_idle_engine);

    // Run main application.
    application->run(argc, argv);

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    Dout(dc::warning, error);
  }

  Dout(dc::notice, "Leaving main()");
}
