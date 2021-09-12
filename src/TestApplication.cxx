#include "sys.h"
#include "vulkan/Defaults.h"
#include "vulkan/OperatingSystem.h"
#include "vulkan/VulkanWindow.h"
#include "statefultask/DefaultMemoryPagePool.h"
#include "threadpool/AIThreadPool.h"
#include "evio/EventLoop.h"
#include "resolver-task/DnsResolver.h"
#include "xcb-task/ConnectionBrokerKey.h"
#include "utils/AIAlert.h"
#include "debug.h"
#ifdef CWDEBUG
#include "utils/debug_ostream_operators.h"
#endif

using namespace linuxviewer;

class Application /*: public vulkan::Application*/
{
};

class Window : public OS::Window
{
  void OnWindowSizeChanged() override
  {
    DoutEntering(dc::notice, "Window::OnWindowSizeChanged()");
  }

  void MouseMove(int x, int y) override
  {
    DoutEntering(dc::notice, "Window::MouseMove(" << x << ", " << y << ")");
  }

  void MouseClick(size_t button, bool pressed) override
  {
    DoutEntering(dc::notice, "Window::MouseClick(" << button << ", " << pressed << ")");
  }

  void ResetMouse() override
  {
    DoutEntering(dc::notice, "Window::ResetMouse()");
  }

#if 0
  void Draw() override
  {
    DoutEntering(dc::notice, "Window::Draw()");
  }

  bool ReadyToDraw() const override
  {
    DoutEntering(dc::notice, "Window::ReadyToDraw()");
    return false;
  }
#endif
};

int main()
{
  Debug(NAMESPACE_DEBUG::init());
  Dout(dc::notice, "Entering main()");

  Application application;

  // Create a AIMemoryPagePool object (must be created before thread_pool).
  [[maybe_unused]] AIMemoryPagePool mpp;

  // Set up the thread pool for the application.
  int const number_of_threads = 8;                      // Use a thread pool of 8 threads.
  int const max_number_of_threads = 16;                 // This can later dynamically be increased to 16 if needed.
  int const queue_capacity = number_of_threads;
  int const reserved_threads = 1;                       // Reserve 1 thread for each priority.
  // Create the thread pool.
  AIThreadPool thread_pool(number_of_threads, max_number_of_threads);
  Debug(thread_pool.set_color_functions([](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; }));
  // And the thread pool queues.
  [[maybe_unused]] AIQueueHandle high_priority_queue   = thread_pool.new_queue(queue_capacity, reserved_threads);
  [[maybe_unused]] AIQueueHandle medium_priority_queue = thread_pool.new_queue(queue_capacity, reserved_threads);
                   AIQueueHandle low_priority_queue    = thread_pool.new_queue(queue_capacity);

  xcb::ConnectionBrokerKey broker_key;

  // Main application begin.
  try
  {
    // Set up the I/O event loop.
    evio::EventLoop event_loop(low_priority_queue COMMA_CWDEBUG_ONLY("\e[36m", "\e[0m"));
    resolver::Scope resolver_scope(low_priority_queue, false);

    // Brokers are not singletons, but this object could be shared between multiple threads.
    auto broker = statefultask::create<task::Broker<task::XcbConnection>>(CWDEBUG_ONLY(false));
    // The broker never finishes, unless abort() is called on it.
    broker->run(low_priority_queue);

    auto window_task = statefultask::create<task::VulkanWindow>(CWDEBUG_ONLY(true));

    // Window initialization.
    window_task->set_window_type<Window>();
    window_task->set_title("TestApplication");
    window_task->set_size({1000, 800});
    //broker_key.set_display_name(":0");
    window_task->set_xcb_connection(broker, &broker_key);

    // Create window and start rendering loop.
    window_task->run();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    // Vulkan preparations and initialization.
//    project.prepare(window.get_parameters());

    // Stop the broker task.
    broker->terminate([](task::XcbConnection* xcb_connection){ xcb_connection->close(); });

    // Application terminated cleanly.
    event_loop.join();
  }
  catch (AIAlert::Error const& error)
  {
    // Application terminated with an error.
    Dout(dc::warning, "\e[31m" << error << ", caught in TestApplication.cxx\e[0m");
  }
#ifndef CWDEBUG // Commented out so we can see in gdb where an exception is thrown from.
  catch (std::exception& exception)
  {
    DoutFatal(dc::core, "\e[31mstd::exception: " << exception.what() << " caught in TestApplication.cxx\e[0m");
  }
#endif
}
