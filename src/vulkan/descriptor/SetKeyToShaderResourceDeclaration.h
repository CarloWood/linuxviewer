#pragma once

#include "SetKey.h"
#include "ShaderResourceDeclaration.h"
#include "vk_utils/print_pointer.h"
#include <map>

namespace vulkan::descriptor {

class SetKeyToShaderResourceDeclaration
{
 private:
  std::map<SetKey, shader_builder::ShaderResourceDeclaration const*> m_set_key_to_shader_resource_declaration;

 public:
  void try_emplace_declaration(SetKey set_key, shader_builder::ShaderResourceDeclaration const* shader_resource_declaration)
  {
    DoutEntering(dc::vulkan, "SetKeyToShaderResourceDeclaration::try_emplace(" << set_key << ", " << vk_utils::print_pointer(shader_resource_declaration) << ") [" << this << "]");
    auto pib = m_set_key_to_shader_resource_declaration.try_emplace(set_key, shader_resource_declaration);
    // Do not call add_*shader_resource* twice for the same shader resource.
    ASSERT(pib.second);
  }

  shader_builder::ShaderResourceDeclaration const* get_shader_resource_declaration(SetKey set_key) const
  {
    auto shader_resource_declaration = m_set_key_to_shader_resource_declaration.find(set_key);
    // Don't call get_shader_resource_declaration for a key that wasn't first passed to try_emplace.
    ASSERT(shader_resource_declaration != m_set_key_to_shader_resource_declaration.end());
    return shader_resource_declaration->second;
  }
};

} // namespace vulkan::identifier
