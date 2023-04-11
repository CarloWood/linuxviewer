#pragma once

#include <vulkan/shader_builder/ShaderVariableLayouts.h>

struct LeftPosition;

LAYOUT_DECLARATION(LeftPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(mat4, unused),
    LAYOUT(SomeStruct, unused2),
    LAYOUT(Float, y)
  );
};

// Struct describing data type and format of uniform block.
STRUCT_DECLARATION(LeftPosition)
{
  MEMBER(0, mat4, unused);
  MEMBER(1, SomeStruct, unused2);
  MEMBER(2, glsl::Float, y);
};
