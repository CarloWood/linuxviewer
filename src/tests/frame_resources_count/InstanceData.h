#pragma once

#include "shaderbuilder/VertexAttribute.h"

struct InstanceData;

LAYOUT_DECLARATION(InstanceData, per_instance_data)
{
  static constexpr auto layouts = make_layouts(
    MEMBER(vec4, m_position)
  );
};

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec4 m_position;
};
