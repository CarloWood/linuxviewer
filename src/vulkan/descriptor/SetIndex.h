#pragma once

#include "utils/Vector.h"

namespace vulkan::descriptor {

// SetIndex
//
// An index into vectors that enumerate DescriptorSets for a given pipeline.
//
// The canonical vector here is .pSetLayouts in vk::PipelineLayoutCreateInfo
// that stores vk::DescriptorSetLayout elements for each DescriptorSet.
//
struct SetIndexCategory;
using SetIndex = utils::VectorIndex<SetIndexCategory>;

} // namespace vulkan::descriptor
