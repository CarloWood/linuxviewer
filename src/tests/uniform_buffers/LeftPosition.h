#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct LeftPosition;

LAYOUT_DECLARATION(LeftPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
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
