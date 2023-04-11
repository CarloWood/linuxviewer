#pragma once
#undef LV_NEEDS_COMMAND_BUFFER_INL_H

#include "VertexBuffers.h"
#include "PushConstantRange.h"
#include "VertexBuffers.inl.h"

namespace vulkan::handle {

void CommandBuffer::bindVertexBuffers(VertexBuffers const& vertex_buffers)
{
  vertex_buffers.bind({}, *this);
}

template<typename T>
void CommandBuffer::pushConstants(vk::PipelineLayout layout, PushConstantRange const& push_constant_range, T const& push_constants)
{
  // The specified `push_constant_range` must exactly match the data passed in `push_constants`.
  //
  // For example, if the application uses a struct PushConstant:
  //
  //     struct PushConstant {
  //       int k;
  //       int n;               // ⎫
  //       float v;             // ⎬ R
  //       float s;             // ⎭
  //       double d;
  //     };
  //
  // and we need to push/update n and s, then push_constant_range must (at least) represent the range R
  // (note that it will also include v in this case) and T must be equal to:
  //
  //     struct Example {
  //       int n;
  //       float v;
  //       float s;
  //     };
  //
  ASSERT(push_constant_range.size() == sizeof(T));

  vk::CommandBuffer::pushConstants(layout, push_constant_range.shader_stage_flags(), push_constant_range.offset(), push_constant_range.size(), &push_constants);
}

} // namespace vulkan::handle
