#pragma once

#include "DeclarationContext.h"
#include "descriptor/SetIndex.h"
#include <cstdint>
#include <map>

namespace vulkan::shaderbuilder {

class ShaderResource;

struct ShaderResourceDeclarationContext final : DeclarationContext
{
 private:
  utils::Vector<uint32_t, descriptor::SetIndex> m_next_binding;
  std::map<ShaderResource const*, uint32_t> m_bindings;

  pipeline::ShaderInputData* const m_owning_shader_input_data;

 public:
  ShaderResourceDeclarationContext(pipeline::ShaderInputData* owning_shader_input_data) :
    m_owning_shader_input_data(owning_shader_input_data) { }

  uint32_t binding(ShaderResource const* shader_resource) const
  {
    return m_bindings.at(shader_resource);
  }

  void update_binding(ShaderResource const* shader_resource);

  void glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, ShaderResource const* shader_resource, pipeline::ShaderInputData* shader_input_data);

  std::string generate_declaration(vk::ShaderStageFlagBits shader_stage) const override;
};

} // namespace vulkan::shaderbuilder
