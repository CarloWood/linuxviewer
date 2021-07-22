#pragma once

#include <vulkan/vulkan.hpp>

namespace vulkan {

// This struct only provides default values, it should not add any members.
struct ApplicationInfo : vk::ApplicationInfo
{
  static constexpr uint32_t    s_default_applicationVersion = 1;
  static constexpr char const* s_default_pEngineName = "LinuxViewer";
  static constexpr uint32_t    s_default_engineVersion = 1;
  static constexpr uint32_t    s_default_apiVersion = VK_API_VERSION_1_1;

  constexpr ApplicationInfo(
      char const* pApplicationName_
      ) : vk::ApplicationInfo(
        pApplicationName_,
        s_default_applicationVersion,
        s_default_pEngineName,
        s_default_engineVersion,
        s_default_apiVersion) { }
};

} // namespace vulkan
