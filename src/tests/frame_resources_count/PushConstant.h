#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct PushConstant;

LAYOUT_DECLARATION(PushConstant, push_constant_std430)
{
  static constexpr auto struct_layout = make_struct_layout(
    MEMBER(vec3, unused),
    MEMBER(Float, aspect_scale)
  );
};

// Struct describing data type and format of push constants.
struct PushConstant
{
  glsl::vec3 unused;
  glsl::Float aspect_scale;
};

static_assert(offsetof(PushConstant, aspect_scale) == std::tuple_element_t<1, decltype(vulkan::shaderbuilder::ShaderVariableLayouts<PushConstant>::struct_layout)::members_tuple>::offset,
    "Offset of aspect_scale is wrong");
