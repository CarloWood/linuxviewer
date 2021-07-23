#pragma once

#include "LvInstance.h"

namespace vulkan {

struct InstanceCreateInfoArgs
{
  static constexpr vk::ArrayProxy<char const* const> default_pEnabledLayerNames = {};
  static constexpr vk::ArrayProxy<char const* const> default_pEnabledExtensionNames = {};

  vk::ArrayProxy<char const* const> const layers = default_pEnabledLayerNames;
  vk::ArrayProxy<char const* const> const extensions = default_pEnabledExtensionNames;
};

struct InstanceCreateInfoArgLists
{
  std::vector<char const*> m_enabled_layer_names;
  std::vector<char const*> m_enabled_extension_names;

  InstanceCreateInfoArgLists(InstanceCreateInfoArgs&& args) :
    m_enabled_layer_names(args.layers.begin(), args.layers.end()),
    m_enabled_extension_names(args.extensions.begin(), args.extensions.end()) { }
};

// This struct only provides default values, it should not add any members.
//
// However, in debug mode two vectors are added with the list of layers and extensions
// that vk::InstanceCreateInfo will point to (after calling add_debug_layer_and_extension()).
// The base class is therefore made protected, forcing the user to configure this
// object exclusively through the provided constructor.
//
// Usage example,
//
#ifdef EXAMPLE_CODE
  vulkan::InstanceCreateInfo const instance_create_info(application_create_info, {
      .layers = { "VK_LAYER_KHRONOS_validation" },
      .extensions = { "VK_EXT_debug_utils", "VK_EXT_display_surface_counter" }
    });
#endif
// Note: it is not necessary (or recommended) to add "VK_LAYER_KHRONOS_validation",
// and "VK_EXT_debug_utils", because those will automatically be added in debug mode.
//
struct InstanceCreateInfo : protected InstanceCreateInfoArgLists, protected vk::InstanceCreateInfo
{
  // The life-time of applicationInfo_, pEnabledLayerNames_ and pEnabledExtensionNames_ must be larger
  // than the life-time of this InstanceCreateInfo, and may not be changed after passing them.
  InstanceCreateInfo(vk::ApplicationInfo const& applicationInfo_, InstanceCreateInfoArgs&& args = {}) :
    InstanceCreateInfoArgLists(std::move(args)),        // Make a copy of the temporary initializer lists that vk::ArrayProxy is pointing to.
    vk::InstanceCreateInfo(
        {},                             // Reserved for future use; flags MUST be zero (VUID-VkInstanceCreateInfo-flags-zerobitmask).
        &applicationInfo_,
        m_enabled_layer_names,
        m_enabled_extension_names)
  {
    Debug(add_debug_layer_and_extension());
  }

  // Only provide non-mutable access to the base class.
  //
  // This is necessary because we own the memory that the base class points to.
  // For example, a call to vk::InstanceCreateInfo::setPEnabledLayerNames should not be possible
  // because in that case we'd lose what add_debug_layer_and_extension added.
  vk::InstanceCreateInfo const& base() const
  {
    return *this;
  }

#ifdef CWDEBUG
 private:
  void add_debug_layer_and_extension();
#endif
};

} // namespace vulkan
