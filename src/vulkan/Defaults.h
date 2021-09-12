// Must be inserted first.
#include "debug_ostream_operators.h"

#ifndef VULKAN_DEFAULTS_H
#define VULKAN_DEFAULTS_H

#include "debug.h"
#include <vulkan/vulkan.hpp>
#include <array>

/*=****************************************************************************
 * Convenience marocs                                                         *
 ******************************************************************************/

#ifndef CWDEBUG

#define VK_DEFAULTS_DECLARE(vk_type) struct vk_type : vk::vk_type
#define VK_DEFAULTS_DEBUG_MEMBERS

#else // CWDEBUG

#define VK_DEFAULTS_DECLARE(vk_type) struct vk_type : vk::vk_type, PrintOn
#define VK_DEFAULTS_DEBUG_MEMBERS \
 protected: \
  virtual void print_members(std::ostream& os, char const* prefix) const;

namespace vk_defaults {
struct PrintOn
{
  void print_on(std::ostream& os) const;
 protected:
  virtual void print_members(std::ostream& os, char const* prefix) const = 0;
};
} // namespace vk_defaults

#if !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
extern channel_ct vkverbose;
extern channel_ct vkinfo;
extern channel_ct vkwarning;
extern channel_ct vkerror;
NAMESPACE_DEBUG_CHANNELS_END
#endif

#endif // CWDEBUG

/*=****************************************************************************
 * Default values                                                             *
 ******************************************************************************/

namespace vk_defaults {

VK_DEFAULTS_DECLARE(ApplicationInfo)
{
  constexpr ApplicationInfo() : vk::ApplicationInfo{
    .pApplicationName        = "Application Name",
    .applicationVersion      = VK_MAKE_VERSION(0, 0, 0),
    .pEngineName             = "LinuxViewer",
    .engineVersion           = VK_MAKE_VERSION(0, 1, 0),
    .apiVersion              = VK_API_VERSION_1_1,
  } { }
  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(InstanceCreateInfo)
{
  static constexpr ApplicationInfo default_application_info;
  static constexpr std::array      default_enabled_extensions = {
    "VK_KHR_surface",
    "VK_KHR_xcb_surface",
    CWDEBUG_ONLY("VK_EXT_debug_utils",)
  };
#ifdef CWDEBUG
  // Enable valiation layers only when in debug mode.
  static constexpr std::array      default_enabled_layers = {
    "VK_LAYER_KHRONOS_validation",
  };
#endif

  InstanceCreateInfo() : vk::InstanceCreateInfo{
    .pApplicationInfo        = &default_application_info,
#ifdef CWDEBUG
    .enabledLayerCount       = default_enabled_layers.size(),
    .ppEnabledLayerNames     = default_enabled_layers.data(),
#endif
    .enabledExtensionCount   = default_enabled_extensions.size(),
    .ppEnabledExtensionNames = default_enabled_extensions.data(),
  } { }
  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(DebugUtilsMessengerCreateInfoEXT)
{
#ifdef CWDEBUG
  static constexpr vk::DebugUtilsMessageTypeFlagsEXT default_messageType =
    vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
#endif

  DebugUtilsMessengerCreateInfoEXT() : vk::DebugUtilsMessengerCreateInfoEXT{
#ifdef CWDEBUG
    .messageSeverity         = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
    .messageType             = default_messageType,
#endif
  }
  {
#ifdef CWDEBUG
    // Also turn on severity bits corresponding to debug channels that are on.
    if (DEBUGCHANNELS::dc::vkwarning.is_on())
      messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    if (DEBUGCHANNELS::dc::vkinfo.is_on())
      messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
    if (DEBUGCHANNELS::dc::vkverbose.is_on())
      messageSeverity |= vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
#endif
  }
  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(Instance)
{
  VK_DEFAULTS_DEBUG_MEMBERS
};

} // namespace vk_defaults

#endif // VULKAN_DEFAULTS_H
