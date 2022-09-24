#pragma once

#include "pipeline/Handle.h"
#include "descriptor/SetIndex.h"
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
 private:
  vk::PipelineLayout m_vh_layout;       // Vulkan handle to the pipeline layout.
  pipeline::Handle m_handle;            // Handle to the pipeline.
  utils::Vector<vk::DescriptorSet, descriptor::SetIndex> m_vhv_descriptor_sets;

 public:
  Pipeline() = default;
  Pipeline(Pipeline&&) = default;
  Pipeline(vk::PipelineLayout vh_layout, pipeline::Handle handle, utils::Vector<vk::DescriptorSet, descriptor::SetIndex> const& vhv_descriptor_sets) :
    m_vh_layout(vh_layout), m_handle(handle), m_vhv_descriptor_sets(vhv_descriptor_sets) { }

  Pipeline& operator=(Pipeline&&) = default;

  // Accessors.
  vk::PipelineLayout layout() const { return m_vh_layout; }
  pipeline::Handle const& handle() const { return m_handle; }
  utils::Vector<vk::DescriptorSet, descriptor::SetIndex> const& vhv_descriptor_sets() const { return m_vhv_descriptor_sets; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
