#pragma once

#include "shaderbuilder/ShaderVariableAttribute.h"
#include "shaderbuilder/VertexShaderInputSet.h"
#include "math/glsl.h"

// Struct describing data type and format of vertex attributes.
struct VertexData
{
  glsl::vec4 m_position;
  glsl::vec2 m_texture_coordinates;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableAttributes<VertexData>
{
  static constexpr vk::VertexInputRate input_rate = vk::VertexInputRate::eVertex;       // This is per vertex data.
  static constexpr std::array<ShaderVariableAttribute, 2> attributes = {{
    { Type::vec4, "VertexData::m_position", offsetof(VertexData, m_position) },
    { Type::vec2, "VertexData::m_texture_coordinates", offsetof(VertexData, m_texture_coordinates) }
  }};
};

} // namespace vulkan::shaderbuilder
