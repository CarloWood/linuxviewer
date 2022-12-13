#include "sys.h"

#include "VertexAttribute.h"
#include "pipeline/AddShaderStage.h"
#include "shader_builder/VertexAttributeDeclarationContext.h"
#include <magic_enum.hpp>
#include <sstream>
#include <functional>
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#include "vk_utils/print_flags.h"
#endif

//#include "pipeline/AddVertexShader.inl"

namespace vulkan::shader_builder {

std::string VertexAttribute::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(glsl_id_full());
  return oss.str();
}

// This is the implementation of the virtual function of ShaderVariable.
// Called from AddShaderStage::preprocess1.
DeclarationContext* VertexAttribute::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::AddShaderVariableDeclaration* add_shader_variable_declaration) const
{
  DoutEntering(dc::notice, "VertexAttribute::is_used_in(" << shader_stage << ", " << add_shader_variable_declaration << ") [" << this << "]");

  pipeline::AddShaderStage* add_shader_stage = static_cast<pipeline::AddShaderStage*>(add_shader_variable_declaration);

  // We must use a single context for all vertex attributes, as the context is used to enumerate the 'location' at which the attributes are stored.
  VertexAttributeDeclarationContext* vertex_attribute_declaration_context = add_shader_stage->vertex_shader_location_context({});

  // Register that this vertex attribute is being used.
  vertex_attribute_declaration_context->glsl_id_full_is_used_in(glsl_id_full(), shader_stage, this);

  // Return the declaration context.
  return vertex_attribute_declaration_context;
}

#ifdef CWDEBUG
void VertexAttribute::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_basic_type:" << m_basic_type <<
      ", m_glsl_id_full:" << NAMESPACE_DEBUG::print_string(m_glsl_id_full) <<
      ", m_offset:" << m_offset;
  if (m_array_size > 0)
    os << ", m_array_size:" << m_array_size;
  os << ", m_binding:" << m_binding;

  os << '}';
}
#endif

} // namespace vulkan::shader_builder
