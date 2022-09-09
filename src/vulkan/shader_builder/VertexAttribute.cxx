#include "sys.h"

#include "VertexAttribute.h"
#include "pipeline/ShaderInputData.h"
#include <magic_enum.hpp>
#include <sstream>
#include <functional>
#ifdef CWDEBUG
#include "vk_utils/PrintPointer.h"
#endif

namespace vulkan::shader_builder {

std::string VertexAttribute::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(glsl_id_str());
  return oss.str();
}

DeclarationContext const& VertexAttribute::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const
{
  DoutEntering(dc::notice, "VertexAttribute::is_used_in(" << shader_stage << ", " << shader_input_data << ") [" << this << "]");

  // We must use a single context for all vertex attributes, as the context is used to enumerate the 'location' at which the attributes are stored.
  VertexAttributeDeclarationContext& vertex_attribute_declaration_context = shader_input_data->vertex_shader_location_context({});

  // Register that this vertex attribute is being used.
  vertex_attribute_declaration_context.glsl_id_str_is_used_in(glsl_id_str(), shader_stage, this, shader_input_data);

  // Return the declaration context.
  return vertex_attribute_declaration_context;
}

#ifdef CWDEBUG
void VertexAttribute::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_basic_type:" << m_basic_type <<
      ", m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_offset:" << m_offset;
  if (m_array_size > 0)
    os << ", m_array_size:" << m_array_size;
  os << ", m_binding:" << m_binding;

  os << '}';
}
#endif

} // namespace vulkan::shader_builder
