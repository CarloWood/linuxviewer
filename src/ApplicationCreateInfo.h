#pragma once

#include "utils/MemoryPagePool.h"
#include "vulkan.old/ApplicationInfo.h"
#ifdef CWDEBUG
#include <iosfwd>
#include <functional>
#include <string>
#endif

// Default values for the creation of an Application object.
// This class also contains the ApplicationInfo that is passed to the vulkan instance.
struct ApplicationCreateInfo : public vulkan::ApplicationInfo
{
  using blocks_t = utils::MemoryPagePool::blocks_t;

  static constexpr size_t      default_block_size = 0x8000;     // bytes
  static constexpr float       default_max_duration = 1.0;      // milliseconds
  static constexpr int         default_number_of_threads = 8;
  static constexpr int         default_max_number_of_threads = 16;
  static constexpr int         default_reserved_threads = 1;
  static constexpr char const* default_application_name = "Application Name";

  // Default values passed to utils::MemoryPagePool.
  size_t block_size = default_block_size;                       // The size of a block as returned by allocate(), in bytes.
  blocks_t minimum_chunk_size = {};                             // The minimum size of internally allocated contiguous memory blocks, in blocks.
  blocks_t maximum_chunk_size = {};                             // The maximum size of internally allocated contiguous memory blocks, in blocks.

  float max_duration = default_max_duration;                    // Try not to spend more time per frame than 1 ms in the idle engine.

  // Set up the thread pool for the application.
  int number_of_threads = default_number_of_threads;            // Use a thread pool of 8 threads.
  int max_number_of_threads = default_max_number_of_threads;    // Which can later dynamically increased to 16 if needed.
  int queue_capacity = number_of_threads;                       // The size of thread pool (ring buffer) queue.
  int reserved_threads = default_reserved_threads;              // Reserve 1 thread for each priority.

  ApplicationCreateInfo() : vulkan::ApplicationInfo(default_application_name) { }

  ApplicationCreateInfo& set_block_size(size_t block_size_)
  {
    block_size = block_size_;
    return *this;
  }

  ApplicationCreateInfo& set_chunk_size(blocks_t minimum_chunk_size_, blocks_t maximum_chunk_size_)
  {
    minimum_chunk_size = minimum_chunk_size_;
    maximum_chunk_size = maximum_chunk_size_;
    return *this;
  }

  ApplicationCreateInfo& set_max_duration(float max_duration_)
  {
    max_duration = max_duration_;
    return *this;
  }

  ApplicationCreateInfo& set_number_of_threads(int number_of_threads_, int max_number_of_threads_, int reserved_threads_)
  {
    number_of_threads = number_of_threads_;
    max_number_of_threads = max_number_of_threads_;
    reserved_threads = reserved_threads_;
    return *this;
  }

  ApplicationCreateInfo& set_queue_capacity(int queue_capacity_)
  {
    queue_capacity = queue_capacity_;
    return *this;
  }

#ifdef CWDEBUG
  // Debug colors.
  std::function<std::string(int color)> thread_pool_color_function = [](int color){ std::string code{"\e[30m"}; code[3] = '1' + color; return code; };
  char const* event_loop_color = "\e[36m";                      // Color used for the debug output of the EventLoop thread.
  char const* color_off_code = "\e[0m";                         // Code to turn off a color.

  ApplicationCreateInfo& set_thread_pool_color_function(std::function<std::string(int color)> thread_pool_color_function_)
  {
    thread_pool_color_function = std::move(thread_pool_color_function_);
    return *this;
  }

  ApplicationCreateInfo& set_event_loop_color(char const* event_loop_color_)
  {
    event_loop_color = event_loop_color_;
    return *this;
  }

  ApplicationCreateInfo& set_color_off_code(char const* color_off_code_)
  {
    color_off_code = color_off_code_;
    return *this;
  }

  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};
