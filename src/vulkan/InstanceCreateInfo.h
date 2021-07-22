#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// This struct only provides default values, it should not add any members.
struct InstanceCreateInfo : vk::InstanceCreateInfo
{
  static constexpr vk::ArrayProxyNoTemporaries<char const* const> const& default_pEnabledLayerNames = {};
  static constexpr vk::ArrayProxyNoTemporaries<char const* const> const& default_pEnabledExtensionNames = {};

  // The life-time of applicationInfo_ must be larger than the life-time of this InstanceCreateInfo,
  // and may not be changed after passing it.
  InstanceCreateInfo(vk::ApplicationInfo const& applicationInfo_) :
    vk::InstanceCreateInfo(
        {},                                     // Reserved for future use; flags MUST be zero (VUID-VkInstanceCreateInfo-flags-zerobitmask).
        &applicationInfo_,
        default_pEnabledLayerNames,
        default_pEnabledExtensionNames) { }
};

} // namespace vulkan
