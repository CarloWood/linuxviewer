#pragma once

#include "DeclarationContext.h"
#include <cstdint>
#include <map>

namespace vulkan::shaderbuilder {

class ShaderResource;

struct ShaderResourceDeclarationContext final : DeclarationContext
{
 private:
  uint32_t m_next_binding = 0;
  std::map<ShaderResource const*, uint32_t> m_bindings;

 public:
  uint32_t binding(ShaderResource const* shader_resource) const
  {
    return m_bindings.at(shader_resource);
  }

  void update_binding(ShaderResource const* shader_resource);

  void glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, ShaderVariable const* shader_variable, pipeline::ShaderInputData* shader_input_data) override;
  std::string generate_declaration(vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shaderbuilder
