#include "sys.h"
#include "SamplerKind.h"
#include "LogicalDevice.h"
#include "GraphicsSettings.h"
#ifdef CWDEBUG
#include "vk_utils/print_flags.h"
#endif

namespace vulkan {

SamplerKind::SamplerKind(LogicalDevice const* logical_device, SamplerKindPOD data) : m_data(data)
{
  // By default set anisotropyEnable to true if we have support for it.
  m_data.anisotropyEnable = logical_device->supports_sampler_anisotropy() ? VK_TRUE : VK_FALSE;
}

vk::SamplerCreateInfo SamplerKind::operator()(GraphicsSettingsPOD const& graphics_settings) const
{
  return {
    .flags                   = m_data.flags,
    .magFilter               = m_data.magFilter,
    .minFilter               = m_data.minFilter,
    .mipmapMode              = m_data.mipmapMode,
    .addressModeU            = m_data.addressModeU,
    .addressModeV            = m_data.addressModeV,
    .addressModeW            = m_data.addressModeW,
    .mipLodBias              = m_data.mipLodBias,
    .anisotropyEnable        = m_data.anisotropyEnable,
    .maxAnisotropy           = graphics_settings.maxAnisotropy,
    .compareEnable           = m_data.compareEnable,
    .compareOp               = m_data.compareOp,
    .minLod                  = m_data.minLod,
    .maxLod                  = m_data.maxLod,
    .borderColor             = m_data.borderColor,
    .unnormalizedCoordinates = m_data.unnormalizedCoordinates
  };
}

#ifdef CWDEBUG
void SamplerKind::print_members(std::ostream& os) const
{
  os << '{';
  os << "flags:" << m_data.flags <<
      ", magFilter:" << m_data.magFilter <<
      ", minFilter:" << m_data.minFilter <<
      ", mipmapMode:" << m_data.mipmapMode <<
      ", addressModeU:" << m_data.addressModeU <<
      ", addressModeV:" << m_data.addressModeV <<
      ", addressModeW:" << m_data.addressModeW <<
      ", mipLodBias:" << m_data.mipLodBias <<
      ", anisotropyEnable:" << m_data.anisotropyEnable <<
      ", compareEnable:" << m_data.compareEnable <<
      ", compareOp:" << m_data.compareOp <<
      ", minLod:" << m_data.minLod <<
      ", maxLod:" << m_data.maxLod <<
      ", borderColor:" << m_data.borderColor <<
      ", unnormalizedCoordinates:" << m_data.unnormalizedCoordinates;
  os << '}';
}
#endif

} // namespace vulkan
