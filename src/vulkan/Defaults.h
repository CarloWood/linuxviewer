// Must be inserted first.
#include "debug_ostream_operators.h"

#ifndef VULKAN_DEFAULTS_H
#define VULKAN_DEFAULTS_H

#include "QueueRequest.h"
#include "utils/encode_version.h"
#include "utils/Array.h"
#include "debug.h"
#include <vulkan/vulkan.hpp>

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

namespace vk_defaults {
void debug_init();
} // namespace vk_defaults

#endif // CWDEBUG

/*=****************************************************************************
 * Default values                                                             *
 ******************************************************************************/

namespace vk_defaults {

VK_DEFAULTS_DECLARE(ApplicationInfo)
{
  static constexpr char const* default_application_name = "Application Name";
  static constexpr uint32_t default_application_version = vk_utils::encode_version(0, 0, 0);

  constexpr ApplicationInfo() : vk::ApplicationInfo{
    .pApplicationName        = default_application_name,
    .applicationVersion      = default_application_version,
    .pEngineName             = "LinuxViewer",
    .engineVersion           = vk_utils::encode_version(0, 1, 0),
    .apiVersion              = VK_API_VERSION_1_2,
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
  static constexpr std::array      default_enabled_layers = {
    "VK_LAYER_KHRONOS_validation"
  };
#endif

  constexpr InstanceCreateInfo() : vk::InstanceCreateInfo{
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

  constexpr DebugUtilsMessengerCreateInfoEXT() : vk::DebugUtilsMessengerCreateInfoEXT{
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

VK_DEFAULTS_DECLARE(DebugUtilsObjectNameInfoEXT)
{
  DebugUtilsObjectNameInfoEXT(VkDebugUtilsObjectNameInfoEXT const& object_name_info)
  {
    vk::DebugUtilsObjectNameInfoEXT::operator=(object_name_info);
  }
  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(PhysicalDeviceFeatures)
{
  PhysicalDeviceFeatures()
  {
    setSamplerAnisotropy(VK_TRUE);
  }

  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(DeviceQueueCreateInfo)
{
  VK_DEFAULTS_DEBUG_MEMBERS
};

VK_DEFAULTS_DECLARE(DeviceCreateInfo)
{
  static constexpr utils::Array<vulkan::QueueRequest, 2> default_queue_requests = {{{
    { .queue_flags = vulkan::QueueFlagBits::eGraphics, .max_number_of_queues = 1 },
    { .queue_flags = vulkan::QueueFlagBits::ePresentation, .max_number_of_queues = 1 }
  }}};
#ifdef CWDEBUG
  // This name reflects the usual place where the handle to the device will be stored.
  static constexpr char const* default_debug_name = "Application::m_vulkan_device";
#endif

  DeviceCreateInfo(PhysicalDeviceFeatures const& physical_device_features)
  {
    setPEnabledFeatures(&physical_device_features);
  }

  VK_DEFAULTS_DEBUG_MEMBERS
};

} // namespace vk_defaults

#endif // VULKAN_DEFAULTS_H
