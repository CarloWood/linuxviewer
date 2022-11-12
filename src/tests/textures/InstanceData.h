#pragma once

#include "shader_builder/VertexAttribute.h"

struct InstanceData;

LAYOUT_DECLARATION(InstanceData, per_instance_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec4, m_position)
  );
};

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec4 m_position;
};
