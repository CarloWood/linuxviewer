#pragma once

#include "shader_builder/ShaderVariableLayouts.h"

static constexpr int top_position_array_size = 32;

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(mat2, unused1),
    LAYOUT(Double[4], unused2),
    LAYOUT(Float[top_position_array_size], x)
  );
};

// Struct describing data type and format of uniform block.
STRUCT_DECLARATION(TopPosition)
{
  MEMBER(0, mat2, unused1);
  MEMBER(1, double[4], unused2);
  MEMBER(2, float[top_position_array_size], x);
};

static_assert(offsetof(TopPosition, x) == std::tuple_element_t<2, decltype(vulkan::shader_builder::ShaderVariableLayouts<TopPosition>::struct_layout)::members_tuple>::offset,
    "Offset of x is wrong.");
