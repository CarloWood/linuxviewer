#pragma once

#include "pipeline/Handle.h"
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

 public:
  Pipeline() = default;
  Pipeline(Pipeline&&) = default;
  Pipeline(vk::PipelineLayout vh_layout, pipeline::Handle handle) : m_vh_layout(vh_layout), m_handle(handle) { }

  Pipeline& operator=(Pipeline&&) = default;

  // Accessors.
  vk::PipelineLayout layout() const { return m_vh_layout; }
  pipeline::Handle const& handle() const { return m_handle; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
