#pragma once

#include "memory/Image.h"

namespace vulkan {

// Data collection used for textures.
struct Texture : public memory::Image
{
  vk::UniqueImageView   m_image_view;
  vk::UniqueSampler     m_sampler;

 public:
  // Used to move-assign later.
  Texture() = default;

  // Use sampler as-is.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::MemoryPropertyFlagBits memory_property,
      vk::UniqueSampler&& sampler
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    memory::Image(logical_device, extent, image_view_kind, memory_property
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_image_view(logical_device->create_image_view(m_vh_image, image_view_kind
        COMMA_CWDEBUG_ONLY(ambifix(".m_image_view")))),
    m_sampler(std::move(sampler))
  {
  }

  // Create sampler too.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::MemoryPropertyFlagBits memory_property,
      SamplerKind const& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, image_view_kind, memory_property,
        logical_device->create_sampler(sampler_kind, graphics_settings COMMA_CWDEBUG_ONLY(ambifix))
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too, allowing to pass an initializer list to construct the SamplerKind (from temporary SamplerKindPOD).
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::MemoryPropertyFlagBits memory_property,
      SamplerKindPOD const&& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, image_view_kind, memory_property,
        { logical_device, std::move(sampler_kind) }, graphics_settings
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Class is move-only.
  Texture(Texture&& rhs) = default;
  Texture& operator=(Texture&& rhs) = default;
};

} // namespace vulkan
