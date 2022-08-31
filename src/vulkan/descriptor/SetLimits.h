#pragma once

#include <cstdint>
#include "debug.h"

namespace vulkan::descriptor {

struct SetLimits
{
  uint32_t maxPerStageDescriptorSamplers;
  uint32_t maxPerStageDescriptorUniformBuffers;
  uint32_t maxPerStageDescriptorStorageBuffers;
  uint32_t maxPerStageDescriptorSampledImages;
  uint32_t maxPerStageDescriptorStorageImages;
  uint32_t maxPerStageDescriptorInputAttachments;
  uint32_t maxPerStageResources;
  uint32_t maxDescriptorSetSamplers;
  uint32_t maxDescriptorSetUniformBuffers;
  uint32_t maxDescriptorSetUniformBuffersDynamic;
  uint32_t maxDescriptorSetStorageBuffers;
  uint32_t maxDescriptorSetStorageBuffersDynamic;
  uint32_t maxDescriptorSetSampledImages;
  uint32_t maxDescriptorSetStorageImages;
  uint32_t maxDescriptorSetInputAttachments;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
