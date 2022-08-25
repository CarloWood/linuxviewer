#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shaderbuilder {
class  DeclarationContext;

class ShaderVariable
{
 public:
  virtual ~ShaderVariable() = default;
  virtual DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const = 0;
  virtual char const* glsl_id_str() const = 0;
  virtual std::string name() const = 0;
};

} // namespace vulkan::shaderbuilder
