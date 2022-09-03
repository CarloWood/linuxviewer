#pragma once

#include "utils/Vector.h"

namespace vulkan::descriptor {

// SetIndex
//
// An index into vectors that enumerate DescriptorSets for a given pipeline.
//
// The canonical vector here is .pSetLayouts in vk::PipelineLayoutCreateInfo
// (the first argument of LogicalDevice::create_pipeline_layout) that stores
// vk::DescriptorSetLayout elements for each DescriptorSet.
//
class SetLayout;
using SetIndex = utils::VectorIndex<vulkan::descriptor::SetLayout>;

} // namespace vulkan::descriptor
