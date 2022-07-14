#pragma once

#include "shaderbuilder/ShaderVariableAttribute.h"
#include "math/glsl.h"

// Struct describing data type and format of instance attributes.
struct InstanceData
{
  glsl::vec4 m_position;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableAttributes<InstanceData>
{
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eInstance;     // This is per instance data.
  static constexpr std::array<ShaderVariableAttribute, 1> attributes = {{
    { Type::vec4, "InstanceData::m_position", offsetof(InstanceData, m_position) }
  }};
};

} // namespace vulkan::shaderbuilder
