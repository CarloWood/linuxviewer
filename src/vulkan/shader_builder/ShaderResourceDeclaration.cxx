#include "sys.h"
#ifdef CWDEBUG
#include "ShaderResourceDeclaration.h"
#include "shader_resource/Base.h"
#include "pipeline/ShaderInputData.h"
#include "debug/vulkan_print_on.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/print_reference.h"
#endif
#include "debug.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderResourceDeclaration::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_glsl_id:\"" << m_glsl_id <<
    "\", m_descriptor_type:" << m_descriptor_type <<
      ", m_set_index_hint:" << m_set_index_hint;
      ", m_binding:";
  if (m_binding == undefined_magic)
    os << "<undefined>";
  else
    os << m_binding;
  os << ", m_shader_resource_variables:" << m_shader_resource_variables <<
      ", m_shader_resource:" << vk_utils::print_reference(m_shader_resource) <<
      ", m_stage_flags:" << m_stage_flags;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
