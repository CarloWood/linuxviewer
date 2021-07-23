#include "sys.h"
#include "InstanceCreateInfo.h"

namespace vulkan {

#ifdef CWDEBUG
void InstanceCreateInfo::add_debug_layer_and_extension()
{
  static constexpr std::array<char const* const, 1> debug_pEnabledLayerNames = { "VK_LAYER_KHRONOS_validation" };
  std::vector<char const*> const debug_pEnabledExtensionNames = LvInstance::getRequiredExtensions();

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
