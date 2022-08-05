#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct PushConstant;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<PushConstant> : glsl::push_constant_std430
{
  using containing_class = PushConstant;
  static constexpr auto members = make_members(
    MEMBER(Float, aspect_scale)
  );
};

// Struct describing data type and format of push constants.
struct PushConstant
{
  glsl::Float aspect_scale;
};
