#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct LeftPosition
{
  glsl::Float y;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<LeftPosition>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::Float, "LeftPosition::y", offsetof(LeftPosition, y) },
  }};
};

} // namespace vulkan::shaderbuilder
