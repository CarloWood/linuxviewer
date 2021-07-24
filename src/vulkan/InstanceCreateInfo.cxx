#include "sys.h"
#include "InstanceCreateInfo.h"
#include <GLFW/glfw3.h>
#include <unordered_set>

namespace vulkan {

std::vector<char const*> InstanceCreateInfo::getRequiredGlfwExtensions()
{
  uint32_t glfwExtensionCount = 0;
  char const** glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<char const*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (s_enableValidationLayers)
  {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void InstanceCreateInfo::hasGflwRequiredInstanceExtensions(std::vector<char const*> const& requiredExtensions)
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

bool InstanceCreateInfo::checkValidationLayerSupport()
{
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (char const* layerName : InstanceCreateInfo::validationLayers)
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

    if (!layerFound)
      return false;
  }

  return true;
}

#ifdef CWDEBUG
void InstanceCreateInfo::add_debug_layer_and_extension()
{
  static constexpr std::array<char const* const, 1> debug_pEnabledLayerNames = { "VK_LAYER_KHRONOS_validation" };
  std::vector<char const*> const debug_pEnabledExtensionNames = getRequiredGlfwExtensions();

  // Add all elements of debug_pEnabledLayerNames to m_enabled_layer_names (that do not already exist).
  for (auto name : debug_pEnabledLayerNames)
    if (std::find(m_enabled_layer_names.begin(), m_enabled_layer_names.end(), name) == m_enabled_layer_names.end())
      m_enabled_layer_names.push_back(name);

  // Add all elements of debug_pEnabledExtensionNames to m_enabled_extension_names (that do not already exist).
  for (auto name : debug_pEnabledExtensionNames)
    if (std::find(m_enabled_extension_names.begin(), m_enabled_extension_names.end(), name) == m_enabled_extension_names.end())
      m_enabled_extension_names.push_back(name);

  // Refresh the pointers and counts.
  setPEnabledLayerNames(m_enabled_layer_names);
  setPEnabledExtensionNames(m_enabled_extension_names);
}
#endif

} // namespace vulkan

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
NAMESPACE_DEBUG_CHANNELS_END
#endif
