#pragma once

#include "BufferParameters.h"

namespace vulkan {

// Struct for handling staging buffer's memory mapping.
struct StagingBufferParameters
{
  BufferParameters          m_buffer;
  void*                     m_pointer;
};

} // namespace vulkan
