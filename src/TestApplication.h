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

  void prepare_physical_device_features(vulkan::PhysicalDeviceFeatures& physical_device_features) const override
  {
    // Use the setters from vk::PhysicalDeviceFeatures.
    physical_device_features.setDepthClamp(true);
  }

  void prepare_logical_device(vulkan::DeviceCreateInfo& device_create_info) const override
  {
    using vulkan::QueueFlagBits;

    device_create_info
    .addQueueRequests({
        .queue_flags = QueueFlagBits::eGraphics,
        .max_number_of_queues = 14,
        .priority = 1.0})
    .addQueueRequests({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 3,
        .priority = 0.3})
    .addQueueRequests({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 2,
        .priority = 0.2})
    ;
  }
};
