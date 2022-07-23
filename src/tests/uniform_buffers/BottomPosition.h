#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct BottomPosition
{
  glsl::Float x;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<BottomPosition>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::Float, "BottomPosition::x", offsetof(BottomPosition, x) },
  }};
};

} // namespace vulkan::shaderbuilder
