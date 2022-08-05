#pragma once

#include "shaderbuilder/ShaderVariableLayouts.h"

struct TopPosition;

template<>
struct vulkan::shaderbuilder::ShaderVariableLayouts<TopPosition> : glsl::uniform_std140
{
  using containing_class = TopPosition;
  static constexpr auto members = make_members(
    MEMBER(mat2, "TopPosition::unused1"),
    MEMBER(Float, "TopPosition::x")
//    MEMBER(/*Double*/Float, "TopPosition::unused2"),
//    MEMBER(Float, "TopPosition::unused3")
  );
};

// Struct describing data type and format of uniform block.
struct TopPosition : glsl::uniform_std140
{
  glsl::mat2 unused1;
  glsl::Float x;
//  /*glsl::Double*/ glsl::Float unused2;
//  glsl::Float unused3;
};
