#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

struct SwapChainSupportDetails
{
  vk::SurfaceCapabilitiesKHR capabilities;
  std::vector<vk::SurfaceFormatKHR> formats;
  std::vector<vk::PresentModeKHR> present_modes;
};

} // namespace vulkan
