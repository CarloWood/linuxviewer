#pragma once

#include "debug.h"
#if CW_DEBUG
#include <vulkan/vk_format_utils.h>

namespace vk_utils {

inline bool format_is_multiplane(vk::Format format)
{
  return FormatIsMultiplane(static_cast<VkFormat>(format));
}

inline bool format_is_compatible(vk::Format format1, vk::Format format2)
{
  return FormatCompatibilityClass(static_cast<VkFormat>(format1)) == FormatCompatibilityClass(static_cast<VkFormat>(format2));
}

} // namespace vk_utils

#endif // CW_DEBUG
