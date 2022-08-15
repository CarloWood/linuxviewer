#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct PushConstant;

LAYOUT_DECLARATION(PushConstant, push_constant_std430)
{
  static constexpr auto layouts = make_layouts(
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

static_assert(offsetof(PushConstant, aspect_scale) == std::get<1>(vulkan::shaderbuilder::ShaderVariableLayouts<PushConstant>::layouts).offset, "Offset of aspect_scale is wrong");
