#pragma once

#include "shaderbuilder/VertexAttribute.h"

struct InstanceData;

LAYOUT_DECLARATION(InstanceData, per_instance_data)
{
  static constexpr auto layouts = make_layouts(
    MEMBER(vec3[5], m_unused),
    MEMBER(vec4[3], m_position)
  );
};

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec3 m_unused[5];
  glsl::vec4 m_position[3];
};

static_assert(offsetof(InstanceData, m_position) == std::get<1>(vulkan::shaderbuilder::ShaderVariableLayouts<InstanceData>::layouts).offset, "Offset of m_position is wrong.");
