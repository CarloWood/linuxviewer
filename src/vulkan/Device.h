#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

struct DeviceCreateInfo;

class Device
{
 private:
  vk::Device m_device_handle;

 public:
  void setup(vk::Instance vulkan_instance, vk::SurfaceKHR surface, DeviceCreateInfo const& device_create_info);
};

} // namespace vulkan
