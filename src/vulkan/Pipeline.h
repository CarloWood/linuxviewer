#pragma once

#include "pipeline/Handle.h"
#include "descriptor/SetIndex.h"
#include "descriptor/FrameResourceCapableDescriptorSet.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace vulkan {

// Pipeline
//
// Represents a vulkan pipeline that was created.
//
class Pipeline
{
 public:
  // Same type as in ShaderInputData.
  using descriptor_set_per_set_index_t = utils::Vector<descriptor::FrameResourceCapableDescriptorSet, descriptor::SetIndex>;
  using raw_descriptor_set_per_set_index_t = utils::Vector<vk::DescriptorSet, descriptor::SetIndex>;
  using descriptor_set_per_set_index_per_frame_resource_t = utils::Vector<raw_descriptor_set_per_set_index_t, FrameResourceIndex>;

 private:
  vk::PipelineLayout m_vh_layout;       // Vulkan handle to the pipeline layout.
  pipeline::Handle m_handle;            // Handle to the pipeline.
  descriptor_set_per_set_index_per_frame_resource_t m_descriptor_set_per_set_index_per_frame_resource;

 public:
  Pipeline() = default;
  Pipeline(Pipeline&&) = default;
  Pipeline(vk::PipelineLayout vh_layout, pipeline::Handle handle, descriptor_set_per_set_index_t const& descriptor_sets) :
    m_vh_layout(vh_layout), m_handle(handle)
  {
    FrameResourceIndex const number_of_frame_resources = descriptor_sets.begin()->number_of_frame_resources();
    descriptor::SetIndex const number_of_set_indexes{descriptor_sets.size()};
    m_descriptor_set_per_set_index_per_frame_resource.resize(number_of_frame_resources.get_value());
    for (FrameResourceIndex frame_index{0}; frame_index < number_of_frame_resources; ++frame_index)
    {
      m_descriptor_set_per_set_index_per_frame_resource[frame_index].resize(number_of_set_indexes.get_value());
      for (descriptor::SetIndex set_index{0}; set_index < number_of_set_indexes; ++set_index)
        m_descriptor_set_per_set_index_per_frame_resource[frame_index][set_index] = descriptor_sets[set_index][frame_index];
    }
  }

  Pipeline& operator=(Pipeline&&) = default;

  // Accessors.
  vk::PipelineLayout layout() const { return m_vh_layout; }
  pipeline::Handle const& handle() const { return m_handle; }
  std::vector<vk::DescriptorSet> const& vhv_descriptor_sets(FrameResourceIndex frame_index) const { return m_descriptor_set_per_set_index_per_frame_resource[frame_index]; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
