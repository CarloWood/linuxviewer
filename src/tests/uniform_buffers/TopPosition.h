#pragma once

#include "shader_builder/ShaderVariableLayouts.h"

static constexpr int top_position_array_size = 32;

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
//    LAYOUT(vec3, unused1),
    LAYOUT(Float, float1),
    LAYOUT(Float, float2),
    LAYOUT(Float, float3),
    LAYOUT(Float[top_position_array_size], x)
  );
};

// Struct describing data type and format of uniform block.
STRUCT_DECLARATION(TopPosition)
{
  //MEMBER(0, vec3, unused1);
  MEMBER(0, Float, float1);
  MEMBER(1, Float, float2);
  MEMBER(2, Float, float3);
  MEMBER(3, float[top_position_array_size], x);
};

static_assert(offsetof(TopPosition, x) == 16, "");
static_assert(offsetof(TopPosition, x) == std::tuple_element_t<3, decltype(vulkan::shader_builder::ShaderVariableLayouts<TopPosition>::struct_layout)::members_tuple>::offset, "Offset of x is wrong.");
