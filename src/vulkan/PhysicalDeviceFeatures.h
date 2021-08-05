#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

class PhysicalDeviceFeatures : public vk::PhysicalDeviceFeatures
{
 public:
  static PhysicalDeviceFeatures const s_default_physical_device_features;

  PhysicalDeviceFeatures()
  {
    setSamplerAnisotropy(VK_TRUE);
  }
};

} // namespace vulkan
