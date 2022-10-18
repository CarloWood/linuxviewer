#pragma once

#include "DeclarationContext.h"
#include "descriptor/SetIndex.h"
#include "descriptor/SetKey.h"
#include "descriptor/SetBindingMap.h"
#include <cstdint>
#include <map>

namespace vulkan::pipeline {
class ShaderInputData;
} // namespace vulkan::pipeline

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;

struct ShaderResourceDeclarationContext final : DeclarationContext
{
 private:
  utils::Vector<uint32_t, descriptor::SetIndexHint> m_next_binding;
  std::map<ShaderResourceDeclaration const*, uint32_t> m_bindings;

  pipeline::ShaderInputData* const m_owning_shader_input_data;
  descriptor::SetBindingMap const* m_set_binding_map;

 public:
  ShaderResourceDeclarationContext(pipeline::ShaderInputData* owning_shader_input_data) :
    m_owning_shader_input_data(owning_shader_input_data) { }

  uint32_t binding(ShaderResourceDeclaration const* shader_resource) const
  {
    return m_bindings.at(shader_resource);
  }

  void reserve_binding(descriptor::SetKey descriptor_set_key);
  void update_binding(ShaderResourceDeclaration const* shader_resource);

  void glsl_id_prefix_is_used_in(std::string glsl_id_prefix, vk::ShaderStageFlagBits shader_stage, ShaderResourceDeclaration const* shader_resource, pipeline::ShaderInputData* shader_input_data);

  void generate1(vk::ShaderStageFlagBits shader_stage) const;
  void set_set_binding_map(descriptor::SetBindingMap const* set_binding_map) { m_set_binding_map = set_binding_map; }
  void add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const override;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::shader_builder
