#pragma once

#include "utils/popcount.h"
#include <vulkan/vulkan.hpp>

namespace vulkan::descriptor {

struct LayoutBindingCompare
{
  bool operator()(vk::DescriptorSetLayoutBinding const& lhs, vk::DescriptorSetLayoutBinding const& rhs) const
  {
    // First sort on those things that are completely incompatible:
    if (lhs.pImmutableSamplers != rhs.pImmutableSamplers)
      return lhs.pImmutableSamplers < rhs.pImmutableSamplers;

    if (lhs.descriptorType != rhs.descriptorType)
      return lhs.descriptorType < rhs.descriptorType;

    if (lhs.stageFlags != rhs.stageFlags)
    {
      uint32_t const popcount_lhs = utils::popcount(static_cast<vk::ShaderStageFlags::MaskType>(lhs.stageFlags));
      uint32_t const popcount_rhs = utils::popcount(static_cast<vk::ShaderStageFlags::MaskType>(rhs.stageFlags));
      if (popcount_lhs != popcount_rhs)
        return popcount_lhs < popcount_rhs;
      return lhs.stageFlags < rhs.stageFlags;
    }

    return lhs.descriptorCount < rhs.descriptorCount;
  }
};

} // namespace vulkan::descriptor
