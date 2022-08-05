#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct BottomPosition;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<BottomPosition> : glsl::uniform_std140
{
  using containing_class = BottomPosition;
  static constexpr auto members = make_members(
    MEMBER(vec2, unused),
    MEMBER(Float, x)
  );
};

// Struct describing data type and format of uniform block.
struct BottomPosition
{
  glsl::vec2 unused;
  glsl::Float x;
};
