#pragma once

#include "debug/VULKAN_KIND_DEBUG_MEMBERS.h"
#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vulkan {

class LogicalDevice;
struct GraphicsSettingsPOD;

// Contains all members of SamplerCreateInfo.
struct SamplerKindPOD
{
  vk::SamplerCreateFlags flags              = {};
  // See https://en.wikipedia.org/wiki/Texture_filtering
  vk::Filter               magFilter        = vk::Filter::eLinear;
  vk::Filter               minFilter        = vk::Filter::eLinear;
  vk::SamplerMipmapMode    mipmapMode       = vk::SamplerMipmapMode::eLinear;
  // Used U, V and W values outside of the [0, 1] range. Possibly because the surface is larger than the texture.
  // We use eClampToBorder as default because that makes most sense when calculating a weighted average at the edge,
  // when the texture fits the surface precisely (together with a borderColor of eFloatTransparentBlack).
  vk::SamplerAddressMode   addressModeU     = vk::SamplerAddressMode::eClampToEdge;
  vk::SamplerAddressMode   addressModeV     = vk::SamplerAddressMode::eClampToEdge;
  vk::SamplerAddressMode   addressModeW     = vk::SamplerAddressMode::eClampToEdge;
  // See sampler.bias on https://www.khronos.org/registry/vulkan/specs/1.2/html/vkspec.html#textures-level-of-detail-operation
  float                    mipLodBias       = {};
  vk::Bool32               anisotropyEnable = {};       // Set constructor.
  vk::Bool32               compareEnable    = VK_FALSE;
  vk::CompareOp            compareOp        = {};
  // See https://www.khronos.org/registry/vulkan/specs/1.2/html/vkspec.html#textures-level-of-detail-operation
  float                    minLod           = {};
  float                    maxLod           = {};
  vk::BorderColor          borderColor      = vk::BorderColor::eFloatTransparentBlack;
  vk::Bool32               unnormalizedCoordinates = VK_FALSE;
};

class SamplerKind
{
 private:
  SamplerKindPOD m_data;

 public:
  SamplerKind(LogicalDevice const* logical_device, SamplerKindPOD data);

  // Convert into an ImageCreateInfo.
  vk::SamplerCreateInfo operator()(GraphicsSettingsPOD const& graphics_settings) const;

  // Accessor.
  SamplerKindPOD const* operator->() const { return &m_data; }

 public:
  VULKAN_KIND_DEBUG_MEMBERS
};

} // namespace vulkan
