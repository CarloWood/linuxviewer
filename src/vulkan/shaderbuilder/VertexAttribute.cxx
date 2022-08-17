#include "sys.h"

#include "VertexAttribute.h"
#include "pipeline/ShaderInputData.h"
#include <magic_enum.hpp>
#include <sstream>
#include <functional>
#ifdef CWDEBUG
#include "vk_utils/PrintPointer.h"
#endif

namespace vulkan::shaderbuilder {

std::string VertexAttribute::name() const
{
  std::ostringstream oss;
  oss << 'v' << std::hash<std::string>{}(glsl_id_str());
  return oss.str();
}

std::string VertexAttribute::declaration(pipeline::ShaderInputData* shader_input_data) const
{
  LocationContext& context = shader_input_data->vertex_shader_location_context();

  std::ostringstream oss;
  ASSERT(context.next_location <= 999); // 3 chars max.
  oss << "layout(location = " << context.next_location << ") in " <<
    type2name(m_layout->m_base_type.scalar_type(), m_layout->m_base_type.rows(), m_layout->m_base_type.cols()) << ' ' << name();
  if (m_layout->m_array_size > 0)
    oss << '[' << m_layout->m_array_size << ']';
  oss << ";\t// " << m_layout->m_glsl_id_str << "\n";
  context.update_location(this);
  return oss.str();
}

#ifdef CWDEBUG
void VertexAttributeLayout::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_base_type:" << m_base_type <<
      ", m_glsl_id_str:" << NAMESPACE_DEBUG::print_string(m_glsl_id_str) <<
      ", m_offset:" << m_offset;
  if (m_array_size > 0)
    os << ", m_array_size:" << m_array_size;

  os << '}';
}

void VertexAttribute::print_on(std::ostream& os) const
{
  os << '{';

  os << "m_layout: " << vk_utils::print_pointer(m_layout) <<
      ", m_binding:" << m_binding;

  os << '}';
}
#endif

} // namespace vulkan::shaderbuilder
