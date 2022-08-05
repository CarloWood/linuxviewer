#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct LeftPosition;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<LeftPosition> : glsl::uniform_std140
{
  using containing_class = LeftPosition;
  static constexpr auto members = make_members(
    MEMBER(mat4, unused),
    MEMBER(Float, y)
  );
};

// Struct describing data type and format of uniform block.
struct LeftPosition
{
  glsl::mat4 unused;
  glsl::Float y;
};
