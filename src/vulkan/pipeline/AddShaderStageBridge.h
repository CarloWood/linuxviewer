#pragma once

#include "debug.h"

namespace vulkan::shader_builder {
class ShaderVariable;
} // namespace vulkan::shader_builder

namespace vulkan::pipeline {
class AddPushConstant;

// This bridges the AddShaderStage to other base classes that are used for a Characteristic user class.
struct AddShaderStageBridge
{
  // Implemented by AddShaderStage.
  virtual void add_shader_variable(shader_builder::ShaderVariable const* shader_variable) { ASSERT(false); AI_NEVER_REACHED }

  // Implemented by AddPushConstant.
  virtual AddPushConstant* convert_to_add_push_constant() { ASSERT(false); AI_NEVER_REACHED }
};

} // namespace vulkan::pipeline
