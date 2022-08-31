#include "sys.h"
#include "SetLimits.h"

namespace vulkan::descriptor {

#ifdef CWDEBUG
void SetLimits::print_on(std::ostream& os) const
{
  os << '{';
  os << "maxPerStageDescriptorSamplers:" << maxPerStageDescriptorSamplers <<
      ", maxPerStageDescriptorUniformBuffers:" << maxPerStageDescriptorUniformBuffers <<
      ", maxPerStageDescriptorStorageBuffers:" << maxPerStageDescriptorStorageBuffers <<
      ", maxPerStageDescriptorSampledImages:" << maxPerStageDescriptorSampledImages <<
      ", maxPerStageDescriptorStorageImages:" << maxPerStageDescriptorStorageImages <<
      ", maxPerStageDescriptorInputAttachments:" << maxPerStageDescriptorInputAttachments <<
      ", maxPerStageResources:" << maxPerStageResources <<
      ", maxDescriptorSetSamplers:" << maxDescriptorSetSamplers <<
      ", maxDescriptorSetUniformBuffers:" << maxDescriptorSetUniformBuffers <<
      ", maxDescriptorSetUniformBuffersDynamic:" << maxDescriptorSetUniformBuffersDynamic <<
      ", maxDescriptorSetStorageBuffers:" << maxDescriptorSetStorageBuffers <<
      ", maxDescriptorSetStorageBuffersDynamic:" << maxDescriptorSetStorageBuffersDynamic <<
      ", maxDescriptorSetSampledImages:" << maxDescriptorSetSampledImages <<
      ", maxDescriptorSetStorageImages:" << maxDescriptorSetStorageImages <<
      ", maxDescriptorSetInputAttachments:" << maxDescriptorSetInputAttachments;
  os << '}';
}
#endif

} // namespace vulkan::descriptor
