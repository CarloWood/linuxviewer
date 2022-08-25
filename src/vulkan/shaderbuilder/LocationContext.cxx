#include "sys.h"
#include "LocationContext.h"
#include "VertexAttribute.h"
#include "pipeline/ShaderInputData.h"
#include <sstream>
#include "debug.h"
#ifdef CWDEBUG
#include "vk_utils/PrintPointer.h"
#endif

namespace vulkan::shaderbuilder {

void VertexAttributeDeclarationContext::update_location(VertexAttribute const* vertex_attribute)
{
  DoutEntering(dc::vulkan, "VertexAttributeDeclarationContext::update_location(@" << *vertex_attribute << ") [" << this << "]");
  locations[vertex_attribute] = next_location;
  int number_of_attribute_indices = vertex_attribute->layout().m_base_type.consumed_locations();
  if (vertex_attribute->layout().m_array_size > 0)
    number_of_attribute_indices *= vertex_attribute->layout().m_array_size;
  Dout(dc::notice, "Changing next_location from " << next_location << " to " << (next_location + number_of_attribute_indices) << ".");
  next_location += number_of_attribute_indices;
}

void VertexAttributeDeclarationContext::glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits CWDEBUG_ONLY(shader_stage), ShaderVariable const* shader_variable, pipeline::ShaderInputData* shader_input_data)
{
  // Vertex attributes should only be used in the vertex shader.
  ASSERT(shader_stage == vk::ShaderStageFlagBits::eVertex);
  update_location(static_cast<VertexAttribute const*>(shader_variable));
}

std::string VertexAttributeDeclarationContext::generate_declaration(vk::ShaderStageFlagBits shader_stage) const
{
  std::ostringstream oss;
  ASSERT(next_location <= 999); // 3 chars max.
  for (auto&& vertex_attribute_location_pair : locations)
  {
    VertexAttribute const* vertex_attribute = vertex_attribute_location_pair.first;
    ShaderVariable const* shader_variable = vertex_attribute;
    uint32_t location = vertex_attribute_location_pair.second;
    VertexAttributeLayout const& layout = vertex_attribute->layout();
    oss << "layout(location = " << location << ") in " <<
      glsl::type2name(layout.m_base_type.scalar_type(), layout.m_base_type.rows(), layout.m_base_type.cols()) << ' ' << shader_variable->name();
    if (layout.m_array_size > 0)
      oss << '[' << layout.m_array_size << ']';
    oss << ";\t// " << layout.m_glsl_id_str << "\n";
  }
  return oss.str();
}

} // namespace vulkan::shaderbuilder
