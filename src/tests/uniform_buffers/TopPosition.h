#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct TopPosition
{
  glsl::Float x;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<TopPosition>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
  }};
};

} // namespace vulkan::shaderbuilder
