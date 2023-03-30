#include "sys.h"
#include "ShaderInfos.h"
#include "debug/debug_ostream_operators.h"

namespace vulkan::shader_builder {

#ifdef CWDEBUG
void ShaderInfoCache::print_on(std::ostream& os) const
{
  os << '{';
  ShaderInfo::print_on(os);
  os << "shader_module:" << *m_shader_module <<
      ", push_constant_ranges:" << m_push_constant_ranges <<
      ", vertex_input_binding_descriptions:" << m_vertex_input_binding_descriptions <<
      ", vertex_input_attribute_descriptions:" << m_vertex_input_attribute_descriptions <<
      ", descriptor_set_layouts:" << m_descriptor_set_layouts;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
