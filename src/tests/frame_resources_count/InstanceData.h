#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct InstanceData;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<InstanceData> : glsl::per_instance_data
{
  using containing_class = InstanceData;
  static constexpr auto members = make_members(
    MEMBER(vec4, m_position)
  );
};

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec4 m_position;
};
