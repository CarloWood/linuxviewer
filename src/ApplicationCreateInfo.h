#pragma once

#include <iosfwd>
#include <functional>
#include <string>

// Default values for the creation of an Application object.
struct ApplicationCreateInfo
{
  float const max_duration = 1.0;                       // Try not to spend more time per frame than 1 ms in the idle engine.

  // Set up the thread pool for the application.
  int const number_of_threads = 8;                      // Use a thread pool of 8 threads.
  int const max_number_of_threads = 16;                 // Which can later dynamically increased to 16 if needed.
  int const queue_capacity = number_of_threads;         // The size of thread pool (ring buffer) queue.
  int const reserved_threads = 1;                       // Reserve 1 thread for each priority.

  // Application initialization.
  char const* const application_name = "Application Name";

#ifdef CWDEBUG
  // Debug colors.
  std::function<std::string(int color)> const thread_pool_color_function = [](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; };
  char const* const event_loop_color = "\e[36m";        // Color used for the debug output of the EventLoop thread.
  char const* const color_off_code = "\e[0m";           // Code to turn off a color.

  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};
