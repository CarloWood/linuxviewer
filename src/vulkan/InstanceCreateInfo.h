#pragma once

#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>
#include "debug.h"

// At least repeat this here, in case it isn't defined in debug.h.
#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan {

// Helper class.
struct InstanceCreateInfoArgs
{
  static constexpr vk::ArrayProxy<char const* const> default_pEnabledLayerNames = {};
  static constexpr vk::ArrayProxy<char const* const> default_pEnabledExtensionNames = {};

  vk::ArrayProxy<char const* const> const layers = default_pEnabledLayerNames;
  vk::ArrayProxy<char const* const> const extensions = default_pEnabledExtensionNames;
};

// Helper class.
struct InstanceCreateInfoArgLists
{
  std::vector<char const*> m_enabled_layer_names;
  std::vector<char const*> m_enabled_extension_names;

  InstanceCreateInfoArgLists(InstanceCreateInfoArgs&& args) :
    m_enabled_layer_names(args.layers.begin(), args.layers.end()),
    m_enabled_extension_names(args.extensions.begin(), args.extensions.end()) { }
};

// This struct provides default values for extra layers and extensions
// (see InstanceCreateInfoArgs; currently none).
//
// The InstanceCreateInfoArgLists base class adds two vectors that with
// the complete lists of layer and extention names that vk::InstanceCreateInfo
// will point to. This guarantees that the lifetime of these lists is
// equal to the lifetime of the vk::InstanceCreateInfo.
//
// In debug mode the Khronos validation layer is added as well as required
// extension(s) for debugging.
//
// Usage example,
//
#ifdef EXAMPLE_CODE
  vulkan::InstanceCreateInfo instance_create_info(application_create_info);

OR

  vulkan::InstanceCreateInfo instance_create_info(application_create_info, {
      .layers = { "VK_LAYER_KHRONOS_validation" },
      .extensions = { "VK_EXT_debug_utils", "VK_EXT_display_surface_counter" }
    });

OR

  vulkan::InstanceCreateInfo instance_create_info(application_create_info, {
      .extensions = { "VK_EXT_debug_utils", "VK_EXT_display_surface_counter" }
    });
#endif
//
// Note: it is not necessary (or recommended) to add "VK_LAYER_KHRONOS_validation",
// and "VK_EXT_debug_utils", because those will automatically be added in debug mode.
//
struct InstanceCreateInfo : protected InstanceCreateInfoArgLists, protected vk::InstanceCreateInfo
{
 public:
#ifdef CWDEBUG
  static constexpr bool s_enableValidationLayers = true;
  static constexpr std::array<char const* const, 1> validationLayers = { "VK_LAYER_KHRONOS_validation" };
#else
  static constexpr bool s_enableValidationLayers = false;
  static constexpr std::array<char const* const, 0> validationLayers;
#endif

  static void hasGflwRequiredInstanceExtensions(std::vector<char const*> const& requiredExtensions);
  static bool checkValidationLayerSupport();

  // The life-time of applicationInfo_, pEnabledLayerNames_ and pEnabledExtensionNames_ must be larger
  // than the life-time of this InstanceCreateInfo, and may not be changed after passing them.
  InstanceCreateInfo(vk::ApplicationInfo const& application_info, InstanceCreateInfoArgs&& args = {}) :
    InstanceCreateInfoArgLists(std::move(args)),        // Make a copy of the temporary initializer lists that vk::ArrayProxy is pointing to.
    vk::InstanceCreateInfo(
        {},                             // Reserved for future use; flags MUST be zero (VUID-VkInstanceCreateInfo-flags-zerobitmask).
        &application_info,
        m_enabled_layer_names,
        m_enabled_extension_names)
  {
    Debug(add_debug_layer_and_extension());
  }

  void add_layers(std::vector<char const*> const&& extra_layers);
  void add_extensions(std::vector<char const*> const&& extra_extensions);

  // Only provide non-mutable access to the base class.
  //
  // This is necessary because we own the memory that the base class points to.
  // For example, a call to vk::InstanceCreateInfo::setPEnabledLayerNames should not be possible
  // because in that case we'd lose what add_debug_layer_and_extension added.
  vk::InstanceCreateInfo const& base() const
  {
    return *this;
  }

  //---------------------------------------------------------------------------
  // InstanceCreateInfoArgLists accessors.

  std::vector<char const*> const& enabled_layer_names() const
  {
    return m_enabled_layer_names;
  }

  std::vector<char const*> const& enabled_extension_names() const
  {
    return m_enabled_extension_names;
  }

  //---------------------------------------------------------------------------

 private:
#ifdef CWDEBUG
  void add_debug_layer_and_extension();
#endif
};

} // namespace vulkan
