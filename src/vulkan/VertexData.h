#pragma once

namespace vulkan {

// Struct describing data type and format of vertex attributes.
struct VertexData
{
  struct PositionData
  {
    float x, y, z, w;
  };

  struct TextureCoordinates
  {
    float u, v;
  };

  PositionData m_position;
  TextureCoordinates m_texture_coordinates;
};

} // namespace vulkan
