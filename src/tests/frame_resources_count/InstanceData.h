#pragma once

#include <vulkan/shader_builder/VertexAttribute.h>

struct InstanceData;

LAYOUT_DECLARATION(InstanceData, per_instance_data)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(vec3[5], m_unused),
    LAYOUT(vec4[3], m_position)
  );
};

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec3 m_unused[5];
  glsl::vec4 m_position[3];
};

static_assert(offsetof(InstanceData, m_position) == std::tuple_element_t<1, decltype(vulkan::shader_builder::ShaderVariableLayouts<InstanceData>::struct_layout)::members_tuple>::offset,
    "Offset of m_position is wrong.");
