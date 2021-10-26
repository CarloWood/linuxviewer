#pragma once

namespace vulkan {

// Struct describing data type and format of vertex attributes.
struct VertexData
{
  struct PositionData
  {
    float x, y, z, w;
  } Position;

  struct TexcoordData
  {
    float u, v;
  } Texcoords;
};

} // namespace vulkan
