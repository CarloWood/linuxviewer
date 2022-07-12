#pragma once

#include "shaderbuilder/UniformAttributes.h"

// Struct describing data type and format of uniform block.
struct TopPosition
{
  float x;
};

namespace vulkan::shaderbuilder {

template<>
struct UniformAttributes<TopPosition>
{
  static constexpr std::array<UniformAttribute, 1> attributes = {{
    { Type::Float, "TopPosition::x", offsetof(TopPosition, x) },
  }};
};

} // namespace vulkan::shaderbuilder
