#pragma once

#include <vulkan/vulkan.hpp>
#include <string>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class  DeclarationContext;

class ShaderVariable
{
 protected:
  char const* const m_glsl_id_full;              // "MyPushConstant::element_name"
                                                //
 public:
  ShaderVariable(char const* glsl_id_full) : m_glsl_id_full(glsl_id_full) { }

  // Accessor.
  char const* glsl_id_full() const { return m_glsl_id_full; }

  std::string prefix() const
  {
    std::string glsl_id_full = m_glsl_id_full;
    return glsl_id_full.substr(0, glsl_id_full.find(':'));
  }

  std::string member_name() const
  {
    return std::strchr(m_glsl_id_full, ':') + 2;
  }

  virtual ~ShaderVariable() = default;
  virtual DeclarationContext const& is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const = 0;
  virtual std::string name() const = 0;
};

} // namespace vulkan::shader_builder
