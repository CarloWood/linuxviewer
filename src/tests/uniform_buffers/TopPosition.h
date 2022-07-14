#pragma once

#include "shaderbuilder/UniformAttributes.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct TopPosition
{
  glsl::Float x;
};

namespace vulkan::shaderbuilder {

template<>
struct UniformAttributes<TopPosition>
{
  static constexpr std::array<ShaderVariableAttribute, 1> attributes = {{
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
  }};
};

} // namespace vulkan::shaderbuilder
