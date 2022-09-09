#pragma once

#include "shader_builder/ShaderVariableLayouts.h"

struct PushConstant;

LAYOUT_DECLARATION(PushConstant, push_constant_std430)
{
  static constexpr auto struct_layout = make_struct_layout(
    MEMBER(Float, pc1),
    MEMBER(Float, pc2),
    MEMBER(Float, aspect_scale),
    MEMBER(Float, pc4),
    MEMBER(Float, pc5)
  );
};

// Struct describing data type and format of push constants.
struct PushConstant
{
  glsl::Float pc1;
  glsl::Float pc2;
  glsl::Float aspect_scale;
  glsl::Float pc4;
  glsl::Float pc5;
};

static_assert(offsetof(PushConstant, aspect_scale) == std::tuple_element_t<2, decltype(vulkan::shader_builder::ShaderVariableLayouts<PushConstant>::struct_layout)::members_tuple>::offset,
    "Offset of aspect_scale is wrong");
