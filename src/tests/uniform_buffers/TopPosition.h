#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct TopPosition;

LAYOUT_DECLARATION(TopPosition, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    MEMBER(mat2, unused1),
//    MEMBER(Double[4], unused2),
    MEMBER(Float, x)
//    MEMBER(Float, "TopPosition::unused3")
  );
};

// Struct describing data type and format of uniform block.
struct TopPosition //: glsl::uniform_std140
{
  glsl::mat2 unused1;
//  glsl::Double unused2[4];
  glsl::Float x;
//  /*glsl::Double*/ glsl::Float unused2;
//  glsl::Float unused3;
};

static_assert(offsetof(TopPosition, x) == std::tuple_element_t<1, decltype(vulkan::shaderbuilder::ShaderVariableLayouts<TopPosition>::struct_layout)::members_tuple>::offset,
    "Offset of x is wrong.");
