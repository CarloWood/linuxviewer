#pragma once

#include "shaderbuilder/ShaderVariableLayout.h"

struct PushConstant;

LAYOUT_DECLARATION(PushConstant, push_constant_std430)
{
  static constexpr auto layouts = make_layouts(
    MEMBER(Float, aspect_scale)
  );
};

// Struct describing data type and format of push constants.
struct PushConstant
{
  glsl::Float aspect_scale;
};
