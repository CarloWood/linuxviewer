#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of uniform block.
struct TopPosition : glsl::uniform_std140
{
  glsl::Float x;
//  /*glsl::dmat4x2*/ glsl::Float unused1;
//  /*glsl::Double*/ glsl::Float unused2;
//  glsl::Float unused3;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<TopPosition> : ShaderVariableLayoutsTraits<TopPosition>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
//    { Type::/*dmat4x2*/Float, "TopPosition::unused1", offsetof(TopPosition, unused1) },
//    { Type::/*Double*/Float, "TopPosition::unused2", offsetof(TopPosition, unused2) },
//    { Type::Float, "TopPosition::unused3", offsetof(TopPosition, unused3) },
  }};
};

} // namespace vulkan::shaderbuilder
