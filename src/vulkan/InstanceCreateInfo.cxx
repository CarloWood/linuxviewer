#include "sys.h"
#include "InstanceCreateInfo.h"
//#include <vulkan/vulkan.hpp>            // Must be included before glfwpp/glfwpp.h in order to get vulkan C++ API support.
//#include <glfwpp/glfwpp.h>
#include <unordered_set>

namespace vulkan {

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

void InstanceCreateInfo::add_layers(std::vector<char const*> const&& extra_layers)
{
  // Add all elements of extra_layers to m_enabled_layer_names (that do not already exist).
  for (auto name : extra_layers)
    if (std::find(m_enabled_layer_names.begin(), m_enabled_layer_names.end(), name) == m_enabled_layer_names.end())
      m_enabled_layer_names.push_back(name);

  // Refresh the pointer and count.
  setPEnabledLayerNames(m_enabled_layer_names);
}

void InstanceCreateInfo::add_extensions(std::vector<char const*> const&& extra_extensions)
{
  // Add all elements of extra_extensions to m_enabled_extension_names (that do not already exist).
  for (auto name : extra_extensions)
    if (std::find(m_enabled_extension_names.begin(), m_enabled_extension_names.end(), name) == m_enabled_extension_names.end())
      m_enabled_extension_names.push_back(name);

  // Refresh the pointer and count.
  setPEnabledExtensionNames(m_enabled_extension_names);
}

#ifdef CWDEBUG
void InstanceCreateInfo::add_debug_layer_and_extension()
{
  add_layers({ "VK_LAYER_KHRONOS_validation" });
  add_extensions({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME });
}
#endif

} // namespace vulkan

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
channel_ct vulkan("VULKAN");
NAMESPACE_DEBUG_CHANNELS_END
#endif
