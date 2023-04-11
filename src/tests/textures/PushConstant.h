#pragma once

#include <vulkan/shader_builder/ShaderVariableLayouts.h>

struct PushConstant;

LAYOUT_DECLARATION(PushConstant, push_constant_std430)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(Float, m_x_position),
    LAYOUT(Int, m_texture_index)
  );
};

// Struct describing data type and format of push constants.
struct PushConstant
{
  glsl::Float m_x_position;
  glsl::Int m_texture_index;
};
