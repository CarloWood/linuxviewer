#pragma once

#include "AddShaderStage.h"
#include "debug.h"

namespace vulkan::pipeline {

// The fragment shader has no extra data involved that not already be derived from the shader code.
class AddFragmentShader : public virtual AddShaderStage
{
};

} // namespace vulkan::pipeline
