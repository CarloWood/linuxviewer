#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

class DynamicLoader
{
 private:
  vk::DynamicLoader m_dynamic_loader;

 public:
  DynamicLoader()
  {
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = m_dynamic_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
  }

  void setup(vk::Instance vulkan_instance)
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkan_instance);
  }

  void setup(vk::Device vulkan_device)
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vulkan_device);
  }
};

} // namespace vulkan
