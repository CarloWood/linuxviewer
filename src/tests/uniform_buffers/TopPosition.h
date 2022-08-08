#pragma once

#include "shaderbuilder/ShaderVariableLayout.h"

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto layouts = make_layouts(
    MEMBER(mat2, unused1),
    MEMBER(Float, x)
//    MEMBER(/*Double*/Float, "TopPosition::unused2"),
//    MEMBER(Float, "TopPosition::unused3")
  );
};

// Struct describing data type and format of uniform block.
struct TopPosition : glsl::uniform_std140
{
  glsl::mat2 unused1;
  glsl::Float x;
//  /*glsl::Double*/ glsl::Float unused2;
//  glsl::Float unused3;
};
