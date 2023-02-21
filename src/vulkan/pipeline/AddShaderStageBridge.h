#pragma once

#include "debug.h"

namespace vulkan::shader_builder {
class ShaderVariable;
class ShaderInfoCache;
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
  virtual void cache_push_constant_ranges(shader_builder::ShaderInfoCache& shader_info_cache) { }
  virtual void restore_push_constant_ranges(shader_builder::ShaderInfoCache const& shader_info_cache) { }

  // Implemented by AddVertexShader.
  virtual void cache_vertex_input_descriptions(shader_builder::ShaderInfoCache& shader_info_cache) { }
  virtual void restore_vertex_input_descriptions(shader_builder::ShaderInfoCache const& shader_info_cache) { }

  void update(shader_builder::ShaderInfoCache& shader_info_cache)
  {
    cache_push_constant_ranges(shader_info_cache);
    cache_vertex_input_descriptions(shader_info_cache);
  }

  void restore_from_cache(shader_builder::ShaderInfoCache const& shader_info_cache)
  {
    restore_push_constant_ranges(shader_info_cache);
    restore_vertex_input_descriptions(shader_info_cache);
  }
};

} // namespace vulkan::pipeline
