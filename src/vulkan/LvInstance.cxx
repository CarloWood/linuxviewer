#include "sys.h"
#include "LvInstance.h"
#include "GLFW/glfw3.h"
#include <string>
#include <cstdint>

namespace vulkan {

LvInstance::~LvInstance()
{
  vkDestroyInstance(m_vulkan_instance, nullptr);
}

void LvInstance::createInstance_old()
{
  if (InstanceCreateInfo::s_enableValidationLayers && !InstanceCreateInfo::checkValidationLayerSupport())
    throw std::runtime_error("validation layers requested, but not available!");

  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "LittleVulkanEngine App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName        = "No Engine";
  appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion         = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo     = &appInfo;

  auto extensions                    = InstanceCreateInfo::getRequiredGlfwExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (InstanceCreateInfo::s_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(InstanceCreateInfo::validationLayers.size());
    createInfo.ppEnabledLayerNames = InstanceCreateInfo::validationLayers.data();
  }

  // Initialize vulkan and obtain a handle to it (m_instance) by creating a VkInstance.
  m_vulkan_instance = vk::createInstance(createInfo);

  InstanceCreateInfo::hasGflwRequiredInstanceExtensions(extensions);
}

} // namespace vulkan
