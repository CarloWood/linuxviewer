#pragma once

#include "shader_builder/ShaderVariableLayouts.h"
#include "debug.h"

struct SomeStruct;

LAYOUT_DECLARATION(SomeStruct, uniform_std140)
{
  static constexpr auto struct_layout = make_struct_layout(
    LAYOUT(Float, unused1)
  );
};

STRUCT_DECLARATION(SomeStruct)
{
  MEMBER(0, Float, unused1);
};
