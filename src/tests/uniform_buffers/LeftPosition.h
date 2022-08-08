#pragma once

#include "shaderbuilder/ShaderVariableLayout.h"

struct LeftPosition;

LAYOUT_DECLARATION(LeftPosition, uniform_std140)
{
  static constexpr auto layouts = make_layouts(
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
