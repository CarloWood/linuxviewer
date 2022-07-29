#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"
#include "shaderbuilder/ShaderVariableLayout.h"
#include "math/glsl.h"

// Struct describing data type and format of push constants.
struct PushConstant : glsl::push_constant_std430
{
  glsl::Float aspect_scale;
};

namespace vulkan::shaderbuilder {

template<>
struct ShaderVariableLayouts<PushConstant> : ShaderVariableLayoutsTraits<PushConstant>
{
  static constexpr std::array<ShaderVariableLayout, 1> layouts = {{
    { Type::Float, "PushConstant::aspect_scale", offsetof(PushConstant, aspect_scale) },
  }};
};

} // namespace vulkan::shaderbuilder
