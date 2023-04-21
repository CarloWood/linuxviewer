#pragma once

#include <vulkan/LogicalDevice.h>

class LogicalDevice : public vulkan::LogicalDevice
{
 public:
  static constexpr int root_window_request_cookie = 1;
};
