#include "sys.h"
#include "ShaderResourceDeclaration.h"
#include "pipeline/ShaderInputData.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_pointer.h"
#include "vk_utils/snake_case.h"
#include <sstream>
#include "debug.h"

namespace vulkan::shader_builder {

DeclarationContext* ShaderResourceVariable::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const
{
  DoutEntering(dc::vulkan, "ShaderResourceVariable::is_used_in(" << shader_stage << ", " << shader_input_data << ") [" << this << "]");

  Dout(dc::always, "Getting set_index_hint() of m_shader_resource_declaration_ptr " << m_shader_resource_declaration_ptr);
  descriptor::SetIndexHint set_index_hint = m_shader_resource_declaration_ptr->set_index_hint();
  Dout(dc::always, "set_index_hint = " << set_index_hint);

  // Register that this shader resource is used in shader_stage.
  m_shader_resource_declaration_ptr->used_in(shader_stage);

  // We use a declaration context per (shader resource) descriptor set as this context is used to enumerate the 'binding =' values.
  auto shader_resource_declaration_context_iter = shader_input_data->set_index_hint_to_shader_resource_declaration_context({}).find(set_index_hint);
  // This set index should already have been inserted by ShaderInputData::add_texture, add_uniform_buffer, etc.
  //
  // FIXME: when does this happen?
  ASSERT(shader_resource_declaration_context_iter != shader_input_data->set_index_hint_to_shader_resource_declaration_context({}).end());
  ShaderResourceDeclarationContext* shader_resource_declaration_context = &shader_resource_declaration_context_iter->second;

  Dout(dc::always, "shader_resource_declaration_context found: " << shader_resource_declaration_context);

  // Register that this shader resource is being used in this set.
  shader_resource_declaration_context->glsl_id_prefix_is_used_in(prefix(), shader_stage, m_shader_resource_declaration_ptr, shader_input_data);

  // Return the declaration context.
  return shader_resource_declaration_context;
}

std::string ShaderResourceVariable::name() const
{
  std::ostringstream oss;
  oss << "u_" << prefix() << "_" << member_name();
  return oss.str();
}

std::string ShaderResourceVariable::substitution() const
{
  switch (m_shader_resource_declaration_ptr->descriptor_type())
  {
    case vk::DescriptorType::eUniformBuffer:
    {
      std::string prefix = this->prefix();
      std::ostringstream oss;
      // The same as the declaration, see ShaderResourceDeclarationContext::generate.
      oss << prefix << '.' << member_name();
      return oss.str();
    }
    default:
      break;
  }
  return name();
}

#ifdef CWDEBUG
void ShaderResourceVariable::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id_full:" << NAMESPACE_DEBUG::print_string(m_glsl_id_full) <<           // From base class ShaderVariable.
     ", m_shader_resource_declaration_ptr:" << vk_utils::print_pointer(m_shader_resource_declaration_ptr);
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
