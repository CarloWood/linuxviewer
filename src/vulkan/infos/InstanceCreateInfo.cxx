#include "sys.h"
#include "InstanceCreateInfo.h"
#include "vk_utils/find_missing_names.h"
#include "utils/AIAlert.h"
#include "utils/print_using.h"
#include "utils/QuotedList.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {

void InstanceCreateInfo::check_instance_layers_availability() const
{
  DoutEntering(dc::vulkan, "InstanceCreateInfo::check_instance_layers_availability()");
  Dout(dc::vulkan, "Required layers:" << print_using(m_enabled_layer_names, QuotedList{}) << ")");

  auto available_layers = vk::enumerateInstanceLayerProperties();

  std::vector<char const*> available_names;
  for (auto&& property : available_layers)
    available_names.push_back(property.layerName);
  Dout(dc::vulkan, "Available instance layers:" << print_using(available_names, QuotedList{}));

  auto missing_layers = vk_utils::find_missing_names(m_enabled_layer_names, available_names);

  if (!missing_layers.empty())
  {
    std::string missing_layers_str;
    std::string prefix;
    for (char const* missing : missing_layers)
    {
      missing_layers_str += prefix + missing;
      prefix = ", ";
    }
    THROW_ALERT("Missing required layer(s) {[MISSING_LAYERS]}", AIArgs("[MISSING_LAYERS]", utils::print_using(missing_layers, utils::QuotedList{})));
  }
}

void InstanceCreateInfo::check_instance_extensions_availability() const
{
  DoutEntering(dc::vulkan, "check_instance_extensions_availability()");
  Dout(dc::vulkan, "Required extensions:" << print_using(m_enabled_extension_names, QuotedList{}) << ")");

  auto available_extensions = vk::enumerateInstanceExtensionProperties(nullptr);

  std::vector<char const*> available_names;
  for (auto&& property : available_extensions)
    available_names.push_back(property.extensionName);
  Dout(dc::vulkan, "Available instance extensions:" << print_using(available_names, QuotedList{}));

  // Check that every extension name in required_extensions is available.
  auto missing_extensions = vk_utils::find_missing_names(m_enabled_extension_names, available_names);

  if (!missing_extensions.empty())
  {
    std::string missing_extensions_str;
    std::string prefix;
    for (char const* missing : missing_extensions)
    {
      missing_extensions_str += prefix + missing;
      prefix = ", ";
    }
    THROW_ALERT("Missing required extension(s) {[MISSING_EXTENSIONS]}", AIArgs("[MISSING_EXTENSIONS]", utils::print_using(missing_extensions, utils::QuotedList{})));
  }
}

void InstanceCreateInfo::add_layers(std::vector<char const*> const& extra_layers)
{
  // Add all elements of extra_layers to m_enabled_layer_names (that do not already exist).
  for (auto name : extra_layers)
    if (std::find(m_enabled_layer_names.begin(), m_enabled_layer_names.end(), name) == m_enabled_layer_names.end())
      m_enabled_layer_names.push_back(name);

  // Refresh the pointer and count.
  setPEnabledLayerNames(m_enabled_layer_names);
}

void InstanceCreateInfo::add_extensions(std::vector<char const*> const& extra_extensions)
{
  // Add all elements of extra_extensions to m_enabled_extension_names (that do not already exist).
  for (auto name : extra_extensions)
    if (std::find(m_enabled_extension_names.begin(), m_enabled_extension_names.end(), name) == m_enabled_extension_names.end())
      m_enabled_extension_names.push_back(name);

  // Refresh the pointer and count.
  setPEnabledExtensionNames(m_enabled_extension_names);
}

#ifdef CWDEBUG
void InstanceCreateInfo::add_debug_extension()
{
  add_extensions({ VK_EXT_DEBUG_UTILS_EXTENSION_NAME });
}
#endif

} // namespace vulkan
