#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct TopPosition
{
  glsl::dmat4x2 unused1;
  glsl::Float x;
  glsl::Double unused2;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<TopPosition>
{
  static constexpr std::array<ShaderVariableLayout, 3> layouts = {{
    { Type::dmat4x2, "TopPosition::unused1", offsetof(TopPosition, unused1) },
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
    { Type::Double, "TopPosition::unused2", offsetof(TopPosition, unused2) },
  }};
};

} // namespace vulkan::shaderbuilder
