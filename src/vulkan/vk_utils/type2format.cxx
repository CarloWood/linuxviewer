#include "sys.h"
#include "type2format.h"

namespace vk_utils {

// The following format must be supported by Vulkan (so no test is necessary):
//
// VK_FORMAT_R8_UNORM
// VK_FORMAT_R8_SNORM
// VK_FORMAT_R8_UINT
// VK_FORMAT_R8_SINT
// VK_FORMAT_R8G8_UNORM
// VK_FORMAT_R8G8_SNORM
// VK_FORMAT_R8G8_UINT
// VK_FORMAT_R8G8_SINT
// VK_FORMAT_R8G8B8A8_UNORM
// VK_FORMAT_R8G8B8A8_SNORM
// VK_FORMAT_R8G8B8A8_UINT
// VK_FORMAT_R8G8B8A8_SINT
// VK_FORMAT_B8G8R8A8_UNORM
// VK_FORMAT_A8B8G8R8_UNORM_PACK32
// VK_FORMAT_A8B8G8R8_SNORM_PACK32
// VK_FORMAT_A8B8G8R8_UINT_PACK32
// VK_FORMAT_A8B8G8R8_SINT_PACK32
// VK_FORMAT_A2B10G10R10_UNORM_PACK32
// VK_FORMAT_R16_UNORM
// VK_FORMAT_R16_SNORM
// VK_FORMAT_R16_UINT
// VK_FORMAT_R16_SINT
// VK_FORMAT_R16_SFLOAT
// VK_FORMAT_R16G16_UNORM
// VK_FORMAT_R16G16_SNORM
// VK_FORMAT_R16G16_UINT
// VK_FORMAT_R16G16_SINT
// VK_FORMAT_R16G16_SFLOAT
// VK_FORMAT_R16G16B16A16_UNORM
// VK_FORMAT_R16G16B16A16_SNORM
// VK_FORMAT_R16G16B16A16_UINT
// VK_FORMAT_R16G16B16A16_SINT
// VK_FORMAT_R16G16B16A16_SFLOAT
// VK_FORMAT_R32_UINT
// VK_FORMAT_R32_SINT
// VK_FORMAT_R32_SFLOAT
// VK_FORMAT_R32G32_UINT
// VK_FORMAT_R32G32_SINT
// VK_FORMAT_R32G32_SFLOAT
// VK_FORMAT_R32G32B32_UINT
// VK_FORMAT_R32G32B32_SINT
// VK_FORMAT_R32G32B32_SFLOAT
// VK_FORMAT_R32G32B32A32_UINT
// VK_FORMAT_R32G32B32A32_SINT
// VK_FORMAT_R32G32B32A32_SFLOAT

vk::Format type2format(vulkan::shader_builder::BasicType glsl_type)
{
  vk::Format format;
  int rows = glsl_type.rows();
  glsl::ScalarIndex scalar_type = glsl_type.scalar_type();
  switch (scalar_type)
  {
    case glsl::eFloat:
      // 32_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sfloat;
          break;
        case 2:
          format = vk::Format::eR32G32Sfloat;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sfloat;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sfloat;
          break;
      }
      break;
    case glsl::eDouble:
      // 64_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR64Sfloat;
          break;
        case 2:
          format = vk::Format::eR64G64Sfloat;
          break;
        case 3:
          format = vk::Format::eR64G64B64Sfloat;
          break;
        case 4:
          format = vk::Format::eR64G64B64A64Sfloat;
          break;
      }
      break;
    case glsl::eBool:
      // 8_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Uint;
          break;
        case 2:
          format = vk::Format::eR8G8Uint;
          break;
        case 3:
          format = vk::Format::eR8G8B8Uint;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Uint;
          break;
      }
      break;
    case glsl::eInt:
      // 32_SINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sint;
          break;
        case 2:
          format = vk::Format::eR32G32Sint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sint;
          break;
      }
      break;
    case glsl::eUint:
      // 32_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Uint;
          break;
        case 2:
          format = vk::Format::eR32G32Uint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Uint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Uint;
          break;
      }
      break;
    case glsl::eInt8:
      // int8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Snorm;
          break;
        case 2:
          format = vk::Format::eR8G8Snorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Snorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Snorm;
          break;
      }
      break;
    case glsl::eUint8:
      // uint8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Unorm;
          break;
        case 2:
          format = vk::Format::eR8G8Unorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Unorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Unorm;
          break;
      }
      break;
    case glsl::eInt16:
      // int16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Snorm;
          break;
        case 2:
          format = vk::Format::eR16G16Snorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Snorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Snorm;
          break;
      }
      break;
    case glsl::eUint16:
      // uint16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Unorm;
          break;
        case 2:
          format = vk::Format::eR16G16Unorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Unorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Unorm;
          break;
      }
      break;
  }
  return format;
}

} // namespace vk_utils
