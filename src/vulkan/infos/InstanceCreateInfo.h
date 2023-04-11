#pragma once

#include "../Defaults.h"
#include <vector>

namespace vulkan {

class InstanceCreateInfo : protected vk_defaults::InstanceCreateInfo
{
 private:
  std::vector<char const*> m_enabled_layer_names;
  std::vector<char const*> m_enabled_extension_names;

 public:
  InstanceCreateInfo(vk::ApplicationInfo const& application_info) :
    m_enabled_layer_names(ppEnabledLayerNames, ppEnabledLayerNames + enabledLayerCount),
    m_enabled_extension_names(ppEnabledExtensionNames, ppEnabledExtensionNames + enabledExtensionCount)
  {
    setPApplicationInfo(&application_info);
    Debug(add_debug_extension());
  }

  void check_instance_layers_availability() const;
  void check_instance_extensions_availability() const;

  void add_layers(std::vector<char const*> const& extra_layers);
  void add_extensions(std::vector<char const*> const& extra_extensions);

  // Only provide non-mutable access to the base class.
  //
  // This is necessary because we own the memory that the base class points to.
  // For example, a call to vk::InstanceCreateInfo::setPEnabledLayerNames should not be possible
  // because in that case we'd lose what add_debug_layer_and_extension added.
  vk::InstanceCreateInfo const& read_access() const
  {
    return *this;
  }

  // Except for this function because Application::Application needs to use it to add the DebugUtilsMessengerCreateInfoEXT to be used during Instance creation.
  using vk::InstanceCreateInfo::setPNext;

  //---------------------------------------------------------------------------
  // Accessors.

  std::vector<char const*> const& enabled_layer_names() const
  {
    return m_enabled_layer_names;
  }

  std::vector<char const*> const& enabled_extension_names() const
  {
    return m_enabled_extension_names;
  }

 private:
#ifdef CWDEBUG
  void add_debug_extension();
#endif
};

} // namespace vulkan
