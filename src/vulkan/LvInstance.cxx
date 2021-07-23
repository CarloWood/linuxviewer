#include "sys.h"
#include "LvInstance.h"
#include "GLFW/glfw3.h"
#include <unordered_set>
#include <string>
#include <cstdint>

namespace vulkan {

LvInstance::~LvInstance()
{
  vkDestroyInstance(m_instance, nullptr);
}

void LvInstance::createInstance()
{
  if (s_enableValidationLayers && !checkValidationLayerSupport()) { throw std::runtime_error("validation layers requested, but not available!"); }

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

  auto extensions                    = getRequiredExtensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (s_enableValidationLayers)
  {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  }

  // Initialize vulkan and obtain a handle to it (m_instance) by creating a VkInstance.
  if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
    throw std::runtime_error("failed to create instance!");

  hasGflwRequiredInstanceExtensions(extensions);
}

bool LvInstance::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (char const* layerName : validationLayers)
  {
    bool layerFound = false;

    for (auto const& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound) { return false; }
  }

  return true;
}

std::vector<char const*> LvInstance::getRequiredExtensions()
{
  uint32_t glfwExtensionCount = 0;
  char const** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (s_enableValidationLayers) { extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); }

  return extensions;
}

void LvInstance::hasGflwRequiredInstanceExtensions(std::vector<char const*> const& requiredExtensions)
{
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  std::unordered_set<std::string> available;
  Dout(dc::vulkan, "Available extensions:");
  {
    CWDEBUG_ONLY(debug::Mark m);
    for (auto const& extension : extensions)
    {
      Dout(dc::vulkan, extension.extensionName);
      available.insert(extension.extensionName);
    }
  }

  Dout(dc::vulkan, "Required extensions:");
  {
    CWDEBUG_ONLY(debug::Mark m);
    for (auto const& required : requiredExtensions)
    {
      Dout(dc::vulkan, required);
      if (available.find(required) == available.end())
      {
        throw std::runtime_error("Missing required glfw extension");
      }
    }
  }
}

} // namespace vulkan

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
NAMESPACE_DEBUG_CHANNELS_END
#endif
