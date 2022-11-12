#pragma once

#include "vulkan/Application.h"

class TextureTest : public vulkan::Application
{
  using vulkan::Application::Application;

 private:
  int thread_pool_number_of_worker_threads() const override
  {
    // Lets use 8 worker threads in the thread pool.
    return 8;
  }

 public:
  std::u8string application_name() const override
  {
    return u8"TextureTest";
  }
};
