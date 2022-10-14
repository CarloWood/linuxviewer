#pragma once

#include "shader_builder/ShaderVariableLayouts.h"

static constexpr int top_position_array_size = 32;

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(Float, unused1),
    LAYOUT(vec3, v)
  );
};

// Struct describing data type and format of uniform block.
STRUCT_DECLARATION(TopPosition)
{
  MEMBER(0, Float, unused1);
  MEMBER(1, vec3, v);
};

static_assert(offsetof(TopPosition, v) + decltype(TopPosition::v)::debug_internal_offset == std::tuple_element_t<1, decltype(vulkan::shader_builder::ShaderVariableLayouts<TopPosition>::struct_layout)::members_tuple>::offset, "Offset of v is wrong.");
