#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {
class ShaderVariable;

struct DeclarationContext
{
  virtual ~DeclarationContext() = default;
  virtual void glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, ShaderVariable const* shader_variable, pipeline::ShaderInputData* shader_input_data) = 0;
  virtual std::string generate_declaration(vk::ShaderStageFlagBits shader_stage) const = 0;
};

} // namespace vulkan::shaderbuilder
