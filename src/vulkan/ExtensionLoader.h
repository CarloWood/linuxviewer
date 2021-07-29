#pragma once

// See "Extensions / Per Device function pointers"
// at https://github.com/KhronosGroup/Vulkan-Hpp/
// for more info.

// VULKAN_HPP_DISPATCH_LOADER_DYNAMIC should be set to 1, from CMakeLists.txt.

#include <vulkan/vulkan.hpp>

namespace vulkan {

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Define a dynamic loader.
class ExtensionLoader
{
 private:
  vk::DynamicLoader m_dynamic_loader;

 public:
  ExtensionLoader()
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

#else // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Define a static loader that (pre)loads everything we use.
class ExtensionLoader
{
 public:
  void setup(vk::Instance);
  void setup(vk::Device) {}
};

#endif // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

} // namespace vulkan
