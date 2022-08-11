#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct BottomPosition;

LAYOUT_DECLARATION(BottomPosition, uniform_std140)
{
  static constexpr auto layouts = make_layouts(
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
