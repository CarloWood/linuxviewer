#pragma once

#include "memory/Image.h"
#include "memory/DataFeeder.h"
#include "descriptor/SetKeyContext.h"

namespace vulkan::shader_builder::shader_resource {

struct Texture : public memory::Image
{
 public:
  static constexpr ImageKind default_image_kind{{
    .format = vk::Format::eR8G8B8A8Unorm,
    .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled
  }};

  static ImageViewKind const s_default_image_view_kind;

 private:
  vk::UniqueImageView   m_image_view;
  vk::UniqueSampler     m_sampler;
#if CW_DEBUG
  vulkan::ImageViewKind const* debug_image_view_kind;
#endif

 public:
  // Used to move-assign later.
  Texture() { }
  ~Texture() { DoutEntering(dc::vulkan, "shader_resource::Texture::~Texture() [" << this << "]"); }

  // Use sampler as-is.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      vk::UniqueSampler&& sampler,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    memory::Image(logical_device, extent, image_view_kind, memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix)),
    m_image_view(logical_device->create_image_view(m_vh_image, image_view_kind
        COMMA_CWDEBUG_ONLY(".m_image_view" + ambifix))),
    m_sampler(std::move(sampler))
    COMMA_DEBUG_ONLY(debug_image_view_kind(&image_view_kind))
  {
    DoutEntering(dc::vulkan, "shader_resource::Texture::Texture(" << logical_device << ", " << extent <<
        ", " << image_view_kind << ", @" << &sampler << ", memory_create_info) [" << this << "]");
  }

  // Use sampler as-is and s_default_image_view_kind.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vk::UniqueSampler&& sampler,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, s_default_image_view_kind, std::move(sampler), memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKind const& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, image_view_kind,
        logical_device->create_sampler(sampler_kind, graphics_settings COMMA_CWDEBUG_ONLY(".m_sampler" + ambifix)),
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too and use s_default_image_view_kind.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      SamplerKind const& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, s_default_image_view_kind,
        logical_device->create_sampler(sampler_kind, graphics_settings COMMA_CWDEBUG_ONLY(".m_sampler" + ambifix)),
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too, allowing to pass an initializer list to construct the SamplerKind (from temporary SamplerKindPOD).
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      vulkan::ImageViewKind const& image_view_kind,
      SamplerKindPOD const&& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, image_view_kind,
        { logical_device, std::move(sampler_kind) }, graphics_settings,
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Create sampler too, allowing to pass an initializer list to construct the SamplerKind (from temporary SamplerKindPOD).
  // Use s_default_image_view_kind.
  Texture(
      LogicalDevice const* logical_device,
      vk::Extent2D extent,
      SamplerKindPOD const&& sampler_kind,
      GraphicsSettingsPOD const& graphics_settings,
      MemoryCreateInfo memory_create_info
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    Texture(logical_device, extent, s_default_image_view_kind,
        { logical_device, std::move(sampler_kind) }, graphics_settings,
        memory_create_info
        COMMA_CWDEBUG_ONLY(ambifix))
  {
  }

  // Class is move-only.
  // Note: do NOT move the Ambifix and/or Base::m_descriptor_set_key!
  // Those are only initialized in-place with the constructor that takes just the Ambifix.
  Texture(Texture&& rhs) : Image(std::move(rhs)), m_image_view(std::move(rhs.m_image_view)), m_sampler(std::move(rhs.m_sampler)), debug_image_view_kind(rhs.debug_image_view_kind) { }
  Texture& operator=(Texture&& rhs)
  {
    this->memory::Image::operator=(std::move(rhs));
    m_image_view = std::move(rhs.m_image_view);
    m_sampler = std::move(rhs.m_sampler);
#if CW_DEBUG
    debug_image_view_kind = rhs.debug_image_view_kind;
#endif
    return *this;
  }

  void update_descriptor_set_single(task::SynchronousWindow const* owning_window, descriptor::FrameResourceCapableDescriptorSet const& descriptor_set, uint32_t binding, int array_element) const;

  void upload(vk::Extent2D extent, vulkan::ImageViewKind const& image_view_kind,
      task::SynchronousWindow const* resource_owner,
      std::unique_ptr<DataFeeder> texture_data_feeder,
      AIStatefulTask* parent, AIStatefulTask::condition_type texture_ready);

  // Same, but use s_default_image_view_kind.
  void upload(vk::Extent2D extent,
      task::SynchronousWindow const* resource_owner,
      std::unique_ptr<DataFeeder> texture_data_feeder,
      AIStatefulTask* parent, AIStatefulTask::condition_type texture_ready)
  {
    upload(extent, s_default_image_view_kind, resource_owner, std::move(texture_data_feeder), parent, texture_ready);
  }

  void release_GPU_resources()
  {
    m_sampler.reset();
    m_image_view.reset();
    destroy();
  }

  // Accessors.
  LogicalDevice const* logical_device() const
  { // m_logical_device is only valid when m_vh_image != VK_NULL_HANDLE.
    ASSERT(m_vh_image);
    return m_logical_device;
  }
  vk::ImageView image_view() const { return *m_image_view; }
  vk::Sampler sampler() const { return *m_sampler; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder::shader_resource
