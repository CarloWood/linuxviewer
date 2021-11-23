#pragma once

#include <vulkan/vulkan.hpp>
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

struct ResourceState
{
  vk::PipelineStageFlags        pipeline_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
  vk::AccessFlags               access_mask = vk::AccessFlagBits::eNoneKHR;
  vk::ImageLayout               layout = vk::ImageLayout::eUndefined;
  uint32_t                      queue_family_index = VK_QUEUE_FAMILY_IGNORED;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // name vulkan
