#pragma once

#include <vulkan/LogicalDevice.h>
#include <vulkan/infos/DeviceCreateInfo.h>

class LogicalDevice : public vulkan::LogicalDevice
{
 public:
  // Every time create_root_window is called a cookie must be passed.
  // This cookie will be passed back to the virtual function ... when
  // querying what presentation queue family to use for that window (and
  // related windows).
  static constexpr int root_window_request_cookie1 = 1;
  static constexpr int transfer_request_cookie = 2;

  LogicalDevice()
  {
    DoutEntering(dc::notice, "LogicalDevice::LogicalDevice() [" << this << "]");
  }

  ~LogicalDevice() override
  {
    DoutEntering(dc::notice, "LogicalDevice::~LogicalDevice() [" << this << "]");
  }

  void prepare_physical_device_features(
      vk::PhysicalDeviceFeatures& features10,
      vk::PhysicalDeviceVulkan11Features& features11,
      vk::PhysicalDeviceVulkan12Features& features12,
      vk::PhysicalDeviceVulkan13Features& features13) const override
  {
    features10.setDepthClamp(true);
  }

#if 1 //FIXME: shouldn't it work when this is commented out too?
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
        .cookies = root_window_request_cookie1})        // This may only be used for window1.
    // {2}
    .addQueueRequest({
        .queue_flags = QueueFlagBits::eTransfer,
        .max_number_of_queues = 2,
        .cookies = transfer_request_cookie})
#ifdef CWDEBUG
    .setDebugName("LogicalDevice");
#endif
    ;
  }
#endif
};
