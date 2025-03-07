#pragma once

#include "ShaderVariable.h"
#include "debug.h"

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;

class ShaderResourceVariable final : public ShaderVariable
{
 private:
  ShaderResourceDeclaration* m_shader_resource_declaration_ptr{};       // Pointer to the corresponding shader resource declaration, that contains for example SetIndex and descriptor type.

 public:
  ShaderResourceVariable(char const* glsl_id_full, ShaderResourceDeclaration* shader_resource_declaration_ptr) : ShaderVariable(glsl_id_full), m_shader_resource_declaration_ptr(shader_resource_declaration_ptr) { }

  // Implement base class interface.
  DeclarationContext* is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::AddShaderStage* add_shader_stage) const override;
  std::string name() const override;
  std::string substitution() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const override;

  // Used by PipelineFactoryGraph.
  ShaderResourceDeclaration const* shader_resource_declaration_ptr() const { return m_shader_resource_declaration_ptr; }
#endif
};

} // namespace vulkan::shader_builder
