#include "sys.h"
#include "pipeline/PipelineFactory.h"
#ifdef CWDEBUG
#include "ShaderResourceDeclaration.h"
#include "ShaderResourceBase.h"
#include "debug/vulkan_print_on.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_reference.h"
#endif
#include "debug.h"

namespace vulkan::shader_builder {

void DescriptorSetLayoutBinding::push_back_descriptor_set_layout_binding(
    task::PipelineFactory* pipeline_factory, descriptor::SetIndexHint set_index_hint) const
{
  pipeline_factory->push_back_descriptor_set_layout_binding(set_index_hint, {
      .binding = m_binding,
      .descriptorType = m_descriptor_type,
      .descriptorCount = (uint32_t)std::abs(m_descriptor_array_size),
      .stageFlags = m_stage_flags,
      .pImmutableSamplers = nullptr
  }, m_binding_flags, m_descriptor_array_size, {});
}

ShaderResourceDeclaration::ShaderResourceDeclaration(std::string glsl_id, vk::DescriptorType descriptor_type, descriptor::SetIndexHint set_index_hint /*, uint32_t offset, uint32_t array_size = 0*/, ShaderResourceBase const& shader_resource) :
  DescriptorSetLayoutBinding(descriptor_type, shader_resource.descriptor_array_size(), shader_resource.binding_flags()),
  m_glsl_id(std::move(glsl_id)), m_set_index_hint(set_index_hint),
  /*m_offset(offset), m_array_size(array_size),*/ m_shader_resource(shader_resource)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "ShaderResourceDeclaration::ShaderResourceDeclaration(\"" << glsl_id << "\", " <<
      descriptor_type << ", " << set_index_hint << ", " << shader_resource << ")");
}

#ifdef CWDEBUG
void DescriptorSetLayoutBinding::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_descriptor_type:" << m_descriptor_type <<
      ", m_binding:";
  if (m_binding == undefined_magic)
    os << "<undefined>";
  else
    os << m_binding;
  os <<
      ", m_stage_flags:" << m_stage_flags <<
      ", m_descriptor_array_size:" << m_descriptor_array_size <<
      ", m_binding_flags:" << m_binding_flags <<
      ", m_cached_set_index:" << m_cached_set_index;
  os << '}';
}

void ShaderResourceDeclaration::print_on(std::ostream& os) const
{
  os << '{';
  DescriptorSetLayoutBinding::print_on(os);
  os << ", m_glsl_id:\"" << m_glsl_id <<
    "\", m_set_index_hint:" << m_set_index_hint <<
      ", m_shader_resource_variables:" << m_shader_resource_variables <<
      ", m_shader_resource:" << vk_utils::print_reference(m_shader_resource);
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
