#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct BottomPosition
{
  glsl::vec2 unused;
  glsl::Float x;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<BottomPosition>
{
  static constexpr std::array<ShaderVariableLayout, 2> layouts = {{
    { Type::vec2, "BottomPosition::unused", offsetof(BottomPosition, unused) },
    { Type::Float, "BottomPosition::x", offsetof(BottomPosition, x) }
  }};
};

} // namespace vulkan::shaderbuilder
