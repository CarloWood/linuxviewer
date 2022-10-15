#include "sys.h"
#include "VertexAttributeDeclarationContext.h"
#include "VertexAttribute.h"
#include "pipeline/ShaderInputData.h"
#include "shader_builder/DeclarationsString.h"
#include <sstream>
#include "debug.h"
#ifdef CWDEBUG
#include "vk_utils/print_pointer.h"
#endif

namespace vulkan::shader_builder {

void VertexAttributeDeclarationContext::update_location(VertexAttribute const* vertex_attribute)
{
  DoutEntering(dc::vulkan, "VertexAttributeDeclarationContext::update_location(@" << *vertex_attribute << ") [" << this << "]");
  m_locations[vertex_attribute] = m_next_location;
  int number_of_attribute_indices = vertex_attribute->basic_type().consumed_locations();
  if (vertex_attribute->array_size() > 0)
    number_of_attribute_indices *= vertex_attribute->array_size();
  Dout(dc::notice, "Changing m_next_location from " << m_next_location << " to " << (m_next_location + number_of_attribute_indices) << ".");
  m_next_location += number_of_attribute_indices;
}

void VertexAttributeDeclarationContext::glsl_id_full_is_used_in(char const* glsl_id_full, vk::ShaderStageFlagBits CWDEBUG_ONLY(shader_stage), VertexAttribute const* vertex_attribute, pipeline::ShaderInputData* shader_input_data)
{
  // Vertex attributes should only be used in the vertex shader.
  ASSERT(shader_stage == vk::ShaderStageFlagBits::eVertex);
  update_location(vertex_attribute);
}

void VertexAttributeDeclarationContext::add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const
{
  std::ostringstream oss;
  ASSERT(m_next_location <= 999); // 3 chars max.
  for (auto&& vertex_attribute_location_pair : m_locations)
  {
    VertexAttribute const* vertex_attribute = vertex_attribute_location_pair.first;
    ShaderVariable const* shader_variable = vertex_attribute;
    uint32_t location = vertex_attribute_location_pair.second;
    oss << "layout(location = " << location << ") in " <<
      glsl::type2name(vertex_attribute->basic_type().scalar_type(), vertex_attribute->basic_type().rows(), vertex_attribute->basic_type().cols()) << ' ' << shader_variable->name();
    if (vertex_attribute->array_size() > 0)
      oss << '[' << vertex_attribute->array_size() << ']';
    oss << ";\t// " << vertex_attribute->glsl_id_full() << "\n";
  }
  declarations_out += oss.str();
}

} // namespace vulkan::shader_builder
