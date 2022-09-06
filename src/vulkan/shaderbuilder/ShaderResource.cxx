#include "sys.h"
#include "ShaderResource.h"
#include "pipeline/ShaderInputData.h"
#include "vk_utils/print_flags.h"
#include <sstream>
#include "debug.h"

namespace vulkan::shaderbuilder {

DeclarationContext const& ShaderResource::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const
{
  DoutEntering(dc::vulkan, "ShaderResource::is_used_in(" << shader_stage << ", " << shader_input_data << ") [" << this << "]");

  // Check if this is a new set.
  shader_input_data->saw_set(m_set_index);

  // We must use a single declaration context for all shader resources as this context is used to enumerate the 'set=' and 'binding=' values.
  ShaderResourceDeclarationContext& declaration_context = shader_input_data->shader_resource_context({});

  // Register that this shader resource is being used.
  declaration_context.glsl_id_str_is_used_in(glsl_id_str(), shader_stage, this, shader_input_data);

  // Return the declaration context.
  return declaration_context;
}

std::string ShaderResource::name() const
{
  std::ostringstream oss;
  oss << "u_" << prefix() << "_" << member_name();
  return oss.str();
}

#ifdef CWDEBUG
void ShaderResource::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_descriptor_type:" << m_descriptor_type;
  os << '}';
}
#endif

} // namespace vulkan::shaderbuilder
