#pragma once

#include "debug.h"
#include <vulkan/utility/vk_format_utils.h>

namespace vk_utils {

#if CW_DEBUG
inline bool format_is_multiplane(vk::Format format)
{
  return FormatIsMultiplane(static_cast<VkFormat>(format));
}

inline bool format_is_compatible(vk::Format format1, vk::Format format2)
{
  return FormatCompatibilityClass(static_cast<VkFormat>(format1)) == FormatCompatibilityClass(static_cast<VkFormat>(format2));
}
#endif // CW_DEBUG

inline uint32_t format_component_count(vk::Format format)
{
  return FormatComponentCount(static_cast<VkFormat>(format));
}

} // namespace vk_utils
