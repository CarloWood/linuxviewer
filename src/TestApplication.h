#pragma once

#include "vulkan/Application.h"
#include "vulkan/PhysicalDeviceFeatures.h"
#include "vulkan/DeviceCreateInfo.h"
#include "vulkan/QueueFlags.h"

class TestApplication : public vulkan::Application
{
  using vulkan::Application::Application;

 private:
  int thread_pool_number_of_worker_threads() const override
  {
    // Lets use 4 worker threads in the thread pool.
    return 4;
  }

  std::string application_name() const override
  {
    return "TestApplication";
  }
};
