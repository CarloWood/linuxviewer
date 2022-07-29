#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of instance attributes.
struct InstanceData : glsl::per_instance_data
{
  glsl::vec4 m_position;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<InstanceData> : ShaderVariableLayoutsTraits<InstanceData>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::vec4, "InstanceData::m_position", offsetof(InstanceData, m_position) }
  }};
};

} // namespace vulkan::shaderbuilder
