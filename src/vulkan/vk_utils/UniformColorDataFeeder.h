#pragma once

#include "../memory/DataFeeder.h"
#include <cstring>

namespace vk_utils {

// Define a DataFeeder that produces a texture of size 1x1, resulting in a uniform color.
struct UniformColorDataFeeder final : public vulkan::DataFeeder
{
 private:
  unsigned char m_color[4];

 public:
  UniformColorDataFeeder(unsigned char R, unsigned char G, unsigned B, unsigned A = 255)
  {
    m_color[0] = R;
    m_color[1] = G;
    m_color[2] = B;
    m_color[3] = A;
  }

  // Size of one chunk.
  uint32_t chunk_size() const override { return sizeof(m_color); }
  // Total number of chunks.
  int chunk_count() const override { return 1; }
  // Next number of chunks.
  int next_batch() override { return 1; }

  void get_chunks(unsigned char* chunk_ptr) override
  {
    std::memcpy(chunk_ptr, m_color, sizeof(m_color));
  }
};

} // namespace vk_utils
