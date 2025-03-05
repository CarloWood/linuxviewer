#pragma once

// See "Extensions / Per Device function pointers"
// at https://github.com/KhronosGroup/Vulkan-Hpp/
// for more info.

// VULKAN_HPP_DISPATCH_LOADER_DYNAMIC should be set to 1, from CMakeLists.txt.

#include <vulkan/vulkan.hpp>

namespace vulkan {

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Define a dynamic loader.
class DispatchLoader
{
 private:
  vk::detail::DynamicLoader m_dynamic_loader;

 public:
  DispatchLoader()
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_dynamic_loader);
  }

  void load(vk::Instance vh_instance)
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vh_instance);
  }

  void load(vk::Instance, vk::Device vh_device)
  {
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vh_device);
  }
};

#else // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

// Define a static loader that (pre)loads everything we use.
class DispatchLoader
{
 public:
  void load(vk::Instance vh_instance);
  void load(vk::Instance vh_instance, vk::Device vh_device);
};

#endif // VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1

} // namespace vulkan
