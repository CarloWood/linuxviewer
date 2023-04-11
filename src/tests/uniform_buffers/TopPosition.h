#pragma once

#include <vulkan/shader_builder/ShaderVariableLayouts.h>
#include "SomeStruct.h"
#include "debug.h"

static constexpr int top_position_array_size = 7;

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(Float, unused1),
    LAYOUT(vec3[top_position_array_size], v),
    LAYOUT(SomeStruct, bar)
  );
};

struct MyUniformBuffer;

LAYOUT_DECLARATION(MyUniformBuffer, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(Float, unused),
    LAYOUT(TopPosition[5], foo)
  );
};

// Struct describing data type and format of uniform block.
STRUCT_DECLARATION(TopPosition)
{
  MEMBER(0, Float, unused1);
  MEMBER(1, vec3[top_position_array_size], v);
  MEMBER(2, SomeStruct, bar);
};

STRUCT_DECLARATION(MyUniformBuffer)
{
  MEMBER(0, Float, unused);
  MEMBER(1, TopPosition[5], foo);
};
