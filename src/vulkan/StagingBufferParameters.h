#pragma once

#include "memory/Buffer.h"

namespace vulkan {

// Struct for handling staging buffer's memory mapping.
struct StagingBufferParameters
{
  memory::Buffer m_buffer;
  void* m_pointer;
};

} // namespace vulkan
