#pragma once

#include "ShaderVariable.h"
#include "debug.h"

namespace vulkan::shader_builder {
class ShaderResourceDeclaration;

class ShaderResourceVariable final : public ShaderVariable
{
 private:
  ShaderResourceDeclaration* m_shader_resource_ptr{};      // Pointer to the corresponding shader resource, that contains for example SetIndex and descriptor type.

 public:
  ShaderResourceVariable(char const* glsl_id_full, ShaderResourceDeclaration* shader_resource_ptr) : ShaderVariable(glsl_id_full), m_shader_resource_ptr(shader_resource_ptr) { }

  // Implement base class interface.
  DeclarationContext* is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const override;
  std::string name() const override;

#ifdef CWDEBUG
 public:
  void print_on(std::ostream& os) const override;
#endif
};

} // namespace vulkan::shader_builder
