#pragma once

#include "pipeline/Handle.h"
#include "descriptor/SetIndex.h"
#include "descriptor/FrameResourceCapableDescriptorSet.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif
#include "debug/DebugSetName.h"

namespace vulkan {

// Pipeline
//
// Represents a vulkan pipeline that was created.
//
class Pipeline
{
 public:
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
  Pipeline(vk::PipelineLayout vh_layout, pipeline::Handle handle, descriptor_set_per_set_index_t const& descriptor_sets, FrameResourceIndex const max_number_of_frame_resources
      COMMA_CWDEBUG_ONLY(LogicalDevice const* logical_device)) :
    m_vh_layout(vh_layout), m_handle(handle)
  {
    size_t const number_of_frame_resources = max_number_of_frame_resources.get_value();
    descriptor::SetIndex const set_index_end{descriptor_sets.size()};
    // Reorder the descriptor sets in memory for fast binding.
    m_descriptor_set_per_set_index_per_frame_resource.resize(number_of_frame_resources);
    for (FrameResourceIndex frame_index{0}; frame_index < max_number_of_frame_resources; ++frame_index)
    {
      m_descriptor_set_per_set_index_per_frame_resource[frame_index].resize(set_index_end.get_value());
      for (descriptor::SetIndex set_index{0}; set_index != set_index_end; ++set_index)
      {
        // All of descriptor_sets must be initialized, even for set indexes that aren't used (skipped).
        ASSERT(descriptor_sets[set_index].is_used());
        if (descriptor_sets[set_index].is_frame_resource())
        {
          vk::DescriptorSet vh_descriptor_set = descriptor_sets[set_index][frame_index];
          m_descriptor_set_per_set_index_per_frame_resource[frame_index][set_index] = vh_descriptor_set;
          DebugSetName(vh_descriptor_set, Ambifix{"Pipeline::m_descriptor_set_per_set_index_per_frame_resource[" + to_string(frame_index) + "][" + to_string(set_index) + "]", as_postfix(this)}, logical_device);
        }
        else
        {
          vk::DescriptorSet vh_descriptor_set = descriptor_sets[set_index];
          // Use the same descriptor set for each frame.
          m_descriptor_set_per_set_index_per_frame_resource[frame_index][set_index] = vh_descriptor_set;
#ifdef CWDEBUG
          if (frame_index.is_zero())
            DebugSetName(vh_descriptor_set, Ambifix{"Pipeline::m_descriptor_set_per_set_index_per_frame_resource[" + to_string(frame_index) + "][" + to_string(set_index) + "]", as_postfix(this)}, logical_device);
#endif
        }
      }
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
