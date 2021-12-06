#pragma once

namespace vk_utils {

inline bool format_is_multiplane(vk::Format format)
{
  // FIXME:
  return false;
}

inline bool format_is_compatible(vk::Format format1, vk::Format format2)
{
  // FIXME:
  return format1 == format2;
}

} // namespace vk_utils
