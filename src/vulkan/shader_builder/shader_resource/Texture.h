#pragma once

#include "Base.h"
#include "ShaderResourceMember.h"
#include "memory/Image.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetKeyContext.h"

namespace vulkan::shader_builder::shader_resource {

namespace detail {

// A wrapper around ShaderResourceMember, which doesn't have
// a default constructor, together with the run-time constructed
// glsl id string, so that below we only need a single memory
// allocation.
class TextureShaderResourceMember
{
 private:
  ShaderResourceMember m_member;
  char m_glsl_id_full[];

  TextureShaderResourceMember(std::string const& glsl_id_full) : m_member(m_glsl_id_full)
  {
    strcpy(m_glsl_id_full, glsl_id_full.data());
  }

 public:
  static std::unique_ptr<TextureShaderResourceMember> create(std::string const& glsl_id_full)
  {
    void* memory = malloc(sizeof(ShaderResourceMember) + glsl_id_full.size() + 1);
    return std::unique_ptr<TextureShaderResourceMember>{new (memory) TextureShaderResourceMember(glsl_id_full)};
  }

  void operator delete(void* p)
  {
    free(p);
  }

  // Accessor.
  ShaderResourceMember const& member() const { return m_member; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace detail

// Data collection used for textures.
struct Texture : public Base, memory::Image
{
 private:
  descriptor::SetKey    m_descriptor_set_key;
  std::unique_ptr<detail::TextureShaderResourceMember> m_member;        // A Texture only has a single "member".
  vk::UniqueImageView   m_image_view;
  vk::UniqueSampler     m_sampler;

 public:
  // Used to move-assign later.
  Texture() = default;
  ~Texture() { DoutEntering(dc::vulkan, "shader_resource::Texture::~Texture() [" << this << "]"); }

  // Use sampler as-is.
  Texture(
      std::string_view glsl_id_full_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::UniqueSampler&& sampler,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    m_descriptor_set_key(descriptor::SetKeyContext::instance()),
    memory::Image(logical_device, extent, image_view_kind, memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_image_view(logical_device->create_image_view(m_vh_image, image_view_kind
        COMMA_CWDEBUG_ONLY(".m_image_view" + ambifix))),
    m_sampler(std::move(sampler))
  {
    DoutEntering(dc::vulkan, "shader_resource::Texture::Texture(\"" << glsl_id_full_postfix << "\", " << logical_device << ", " << extent <<
        ", " << image_view_kind << ", @" << &sampler << ", memory_create_info) [" << this << "]");
    std::string glsl_id_full("Texture::");
    glsl_id_full.append(glsl_id_full_postfix);
    m_member = detail::TextureShaderResourceMember::create(glsl_id_full);
  }

  // Create sampler too.
  Texture(
      char const* glsl_id_full_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKind const& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(glsl_id_full_postfix, logical_device, extent, image_view_kind,
        logical_device->create_sampler(sampler_kind, graphics_settings COMMA_CWDEBUG_ONLY(".m_sampler" + ambifix)),
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too, allowing to pass an initializer list to construct the SamplerKind (from temporary SamplerKindPOD).
  Texture(
      char const* glsl_id_full_postfix,
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKindPOD const&& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(glsl_id_full_postfix, logical_device, extent, image_view_kind,
        { logical_device, std::move(sampler_kind) }, graphics_settings,
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Class is move-only.
  Texture(Texture&& rhs) = default;
  Texture& operator=(Texture&& rhs) = default;

  // Accessors.
  char const* glsl_id_full() const { return m_member->member().glsl_id_full(); }
  ShaderResourceMember const& member() const { return m_member->member(); }
  vk::ImageView image_view() const { return *m_image_view; }
  vk::Sampler sampler() const { return *m_sampler; }

  descriptor::SetKey descriptor_set_key() const { return m_descriptor_set_key; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder::shader_resource
