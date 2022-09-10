#include "sys.h"
#include "ShaderResource.h"
#include "pipeline/ShaderInputData.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/PrintPointer.h"
#include <sstream>
#include "debug.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderResource::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id_prefix:\"" << m_glsl_id_prefix <<
    "\", m_descriptor_type:" << m_descriptor_type <<
      ", m_set_index:" << m_set_index;
  os << '}';
}
#endif

DeclarationContext const& ShaderResourceMember::is_used_in(vk::ShaderStageFlagBits shader_stage, pipeline::ShaderInputData* shader_input_data) const
{
  DoutEntering(dc::vulkan, "ShaderResourceMember::is_used_in(" << shader_stage << ", " << shader_input_data << ") [" << this << "]");

  descriptor::SetIndex set_index = m_shader_resource->set_index();

  // Check if this is a new set.
  shader_input_data->saw_set(set_index);

  // We use a declaration context per (shader resource) descriptor set as this context is used to enumerate the 'binding =' values.
  auto shader_resource_declaration_context_iter = shader_input_data->set_index_to_shader_resource_declaration_context({}).find(set_index);
  // This set index should already have been inserted by ShaderInputData::add_texture, add_uniform_buffer, etc.
  //
  // FIXME: when does this happen?
  ASSERT(shader_resource_declaration_context_iter != shader_input_data->set_index_to_shader_resource_declaration_context({}).end());
  ShaderResourceDeclarationContext& shader_resource_declaration_context = shader_resource_declaration_context_iter->second;

#if 0 // debug code
  DeclarationContext const* ptr = &declaration_context;
  Dout(dc::always, "Got declaration context pointer: " << ptr);

  if (!NAMESPACE_DEBUG::being_traced())
  {
    static std::once_flag flag;
    std::call_once(flag, [](){ Debug(attach_gdb()); });
  }
#endif

  // Register that this shader resource is being used in this set.
  shader_resource_declaration_context.glsl_id_prefix_is_used_in(prefix(), shader_stage, m_shader_resource, shader_input_data);

  // Return the declaration context.
  return shader_resource_declaration_context;
}

std::string ShaderResourceMember::name() const
{
  std::ostringstream oss;
  oss << "u_" << prefix() << "_" << member_name();
  return oss.str();
}

#ifdef CWDEBUG
void ShaderResourceMember::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id_full:" << NAMESPACE_DEBUG::print_string(m_glsl_id_full) <<
      ", m_shader_resource:" << vk_utils::print_pointer(m_shader_resource);
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
