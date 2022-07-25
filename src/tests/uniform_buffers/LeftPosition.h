#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct LeftPosition
{
  glsl::mat4 unused;
  glsl::Float y;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<LeftPosition>
{
  static constexpr std::array<ShaderVariableLayout, 2> layouts = {{
    { Type::mat4, "LeftPosition::unused", offsetof(LeftPosition, unused) },
    { Type::Float, "LeftPosition::y", offsetof(LeftPosition, y) },
  }};
};

} // namespace vulkan::shaderbuilder
