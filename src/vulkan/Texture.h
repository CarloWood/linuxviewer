#pragma once

#include "memory/Image.h"

namespace vulkan {

// Data collection used for textures.
struct Texture : public memory::Image
{
 private:
  std::string           m_glsl_id_str;
  vk::UniqueImageView   m_image_view;
  vk::UniqueSampler     m_sampler;

 public:
  // Used to move-assign later.
  Texture() = default;

  // Use sampler as-is.
  Texture(
      std::string_view glsl_id_str_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::UniqueSampler&& sampler,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    m_glsl_id_str(std::string{"Texture::"}.append(glsl_id_str_postfix)),
    memory::Image(logical_device, extent, image_view_kind, memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_image_view(logical_device->create_image_view(m_vh_image, image_view_kind
        COMMA_CWDEBUG_ONLY(".m_image_view" + ambifix))),
    m_sampler(std::move(sampler))
  {
  }

  // Create sampler too.
  Texture(
      char const* glsl_id_str_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKind const& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(glsl_id_str_postfix, logical_device, extent, image_view_kind,
        logical_device->create_sampler(sampler_kind, graphics_settings COMMA_CWDEBUG_ONLY(".m_sampler" + ambifix)),
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too, allowing to pass an initializer list to construct the SamplerKind (from temporary SamplerKindPOD).
  Texture(
      char const* glsl_id_str_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKindPOD const&& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(glsl_id_str_postfix, logical_device, extent, image_view_kind,
        { logical_device, std::move(sampler_kind) }, graphics_settings,
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Class is move-only.
  Texture(Texture&& rhs) = default;
  Texture& operator=(Texture&& rhs) = default;

  // Accessors.
  std::string const& glsl_id_str() const { return m_glsl_id_str; }
  vk::ImageView image_view() const { return *m_image_view; }
  vk::Sampler sampler() const { return *m_sampler; }
};

} // namespace vulkan
