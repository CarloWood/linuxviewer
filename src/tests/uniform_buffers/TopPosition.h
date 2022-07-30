#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct TopPosition : glsl::uniform_std140
{
  glsl::dmat4x2 unused1;
  glsl::Float x;
//  /*glsl::Double*/ glsl::Float unused2;
//  glsl::Float unused3;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<TopPosition> : ShaderVariableLayoutsTraits<TopPosition>
{
  static constexpr std::array<ShaderVariableLayout, 2> layouts = {{
    { Type::dmat4x2, "TopPosition::unused1", offsetof(TopPosition, unused1) },
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
//    { Type::/*Double*/Float, "TopPosition::unused2", offsetof(TopPosition, unused2) },
//    { Type::Float, "TopPosition::unused3", offsetof(TopPosition, unused3) },
  }};
};

} // namespace vulkan::shaderbuilder
