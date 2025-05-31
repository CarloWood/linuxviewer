#pragma once

#include "utils/Vector.h"
#include "debug.h"

NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct setindexhint;
NAMESPACE_DEBUG_CHANNELS_END

namespace vulkan::descriptor {

// SetIndexHint
//
// A temporary descriptor set index that has a bijective relationship with SetIndex,
// which is only determined at set layout realization.
//
struct SetIndexHintCategory;
using SetIndexHint = utils::VectorIndex<SetIndexHintCategory>;

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
