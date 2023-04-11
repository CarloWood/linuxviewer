#pragma once

#include <vulkan/shader_builder/VertexAttribute.h>

struct VertexData;

LAYOUT_DECLARATION(VertexData, per_vertex_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec4, m_position),
    LAYOUT(vec2, m_texture_coordinates)
  );
};

// Struct describing data type and format of vertex attributes.
struct VertexData
{
  glsl::vec4 m_position;
  glsl::vec2 m_texture_coordinates;
};
