#pragma once

#include "VertexBuffers.h"
#include "VertexBuffers.inl.h"

namespace vulkan::handle {

void CommandBuffer::bindVertexBuffers(VertexBuffers const& vertex_buffers)
{
  vertex_buffers.bind({}, *this);
}

} // namespace vulkan::handle
