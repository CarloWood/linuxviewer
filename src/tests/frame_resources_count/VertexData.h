#pragma once

#include "shaderbuilder/VertexAttribute.h"

struct VertexData;

LAYOUT_DECLARATION(VertexData, per_vertex_data)
{
  static constexpr auto layouts = make_layouts(
    MEMBER(vec4[3], m_position),
    MEMBER(vec2, m_texture_coordinates)
  );
};

// Struct describing data type and format of vertex attributes.
struct VertexData
{
  glsl::vec4 m_position[3];
  glsl::vec2 m_texture_coordinates;
};
