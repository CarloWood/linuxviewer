#pragma once

namespace vk_utils {

inline constexpr uint32_t encode_version(uint32_t major, int32_t minor, uint32_t patch)
{
  return (major << 22) | (minor << 12) | patch;
}

inline constexpr uint32_t version_major(uint32_t encoded_version)
{
  return encoded_version >> 22;
}

inline constexpr uint32_t version_minor(uint32_t encoded_version)
{
  return (encoded_version >> 12) & ~(1 << 22);
}

inline constexpr uint32_t version_patch(uint32_t encoded_version)
{
  return encoded_version & ~(1 << 12);
}

} // namespace vk_utils
