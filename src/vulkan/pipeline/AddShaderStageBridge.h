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
  // Make the destructor virtual even though we never delete by AddShaderStageBridge*.
  virtual ~AddShaderStageBridge() = default;

  // Implemented by AddShaderStage.
  virtual void add_shader_variable(shader_builder::ShaderVariable const* shader_variable)
  {
    // This would happen, for example, if you derive from AddPushConstant without also deriving from AddShaderStage.
    ASSERT(false);
    AI_NEVER_REACHED
  }

  // Implemented by AddPushConstant.
  virtual AddPushConstant* convert_to_add_push_constant() { ASSERT(false); AI_NEVER_REACHED }
};

} // namespace vulkan::pipeline
