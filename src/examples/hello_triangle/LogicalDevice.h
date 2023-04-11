#pragma once

#include <vulkan/LogicalDevice.h>

class LogicalDevice : public vulkan::LogicalDevice
{
 public:
  // Every time create_root_window is called a cookie must be passed.
  // This cookie will be passed back to the virtual function ... when
  // querying what presentation queue family to use for that window (and
  // related windows).
  static constexpr int root_window_request_cookie = 1;
  static constexpr int transfer_request_cookie = 2;

  void prepare_logical_device(vulkan::DeviceCreateInfo& device_create_info) const override
  {
    using vulkan::QueueFlagBits;

    device_create_info
    // {0}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eGraphics,
        .max_number_of_queues = 1})
    // {1}
    .combineQueueRequest({
        .queue_flags = QueueFlagBits::ePresentation,
        .max_number_of_queues = 1,                      // Only used when it can not be combined.
        .cookies = root_window_request_cookie})         // This may only be used for window1.
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eTransfer,
        .max_number_of_queues = 2,
        .cookies = transfer_request_cookie})
#ifdef CWDEBUG
    .setDebugName("LogicalDevice");
#endif
    ;
  }
};
