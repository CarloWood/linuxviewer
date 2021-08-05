#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

struct DeviceCreateInfo;

class Device
{
 private:
  vk::PhysicalDevice m_physical_device;         // The underlaying physical device.
  vk::Device m_device_handle;                   // A handle to the logical device.

 public:
  void setup(vk::Instance vulkan_instance, vk::SurfaceKHR surface, DeviceCreateInfo&& device_create_info);
};

} // namespace vulkan
