#pragma once

#include <vulkan/vulkan.hpp>

#ifdef CWDEBUG

class DebugMessenger
{
 private:
  vk::Instance m_vulkan_instance;
  VkDebugUtilsMessengerEXT debugMessenger;

 public:
  DebugMessenger() = default;
  ~DebugMessenger();

  void setup(vk::Instance vulkan_instance);
};

#endif
