#include "sys.h"
#include "AddVertexShader.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "vk_utils/type2format.h"
#include "debug.h"

namespace vulkan::pipeline {

void AddVertexShader::copy_vertex_input_binding_descriptions()
{
  if (m_vertex_shader_input_sets_changed)
  {
    DoutEntering(dc::vulkan, "AddVertexShader::copy_vertex_input_binding_descriptions() [" << this << "]");

    m_vertex_input_binding_descriptions.clear();
    uint32_t binding = 0;
    for (auto const* vextex_input_set : m_vertex_shader_input_sets)
    {
      m_vertex_input_binding_descriptions.push_back({
          .binding = binding,
          .stride = vextex_input_set->chunk_size(),
          .inputRate = vextex_input_set->input_rate()});
      ++binding;
    }
    m_vertex_shader_input_sets_changed = false;
  }
}

void AddVertexShader::copy_vertex_input_attribute_descriptions()
{
  if (m_glsl_id_full_to_vertex_attribute_changed)
  {
    DoutEntering(dc::vulkan, "AddVertexShader::copy_vertex_input_attribute_descriptions() [" << this << "]");

    m_vertex_input_attribute_descriptions.clear();
    for (auto vertex_attribute_iter = m_glsl_id_full_to_vertex_attribute.begin(); vertex_attribute_iter != m_glsl_id_full_to_vertex_attribute.end(); ++vertex_attribute_iter)
    {
      shader_builder::VertexAttribute const& vertex_attribute = vertex_attribute_iter->second;
      auto iter = m_vertex_shader_location_context.locations().find(&vertex_attribute);
      if (iter == m_vertex_shader_location_context.locations().end())
      {
        Dout(dc::warning|continued_cf, "Could not find " << (void*)&vertex_attribute << " (" << vertex_attribute.glsl_id_full() <<
            ") in m_vertex_shader_location_context.locations(), which contains only {");
        for (auto&& e : m_vertex_shader_location_context.locations())
          Dout(dc::continued, "{" << e.first << " (" << e.first->glsl_id_full() << "), location:" << e.second << "}");
        Dout(dc::finish, "}");
        continue;
      }
      // Should have been added by the call to context.update_location(this) in VertexAttributeDeclarationContext::glsl_id_full_is_used_in()
      // in turn called by AddShaderStage::preprocess1.
      ASSERT(iter != m_vertex_shader_location_context.locations().end());
      uint32_t location = iter->second;

      uint32_t const binding = static_cast<uint32_t>(vertex_attribute.binding().get_value());
      vk::Format const format = vk_utils::type2format(vertex_attribute.basic_type());
      uint32_t offset = vertex_attribute.offset();
      int count = vertex_attribute.array_size();          // Zero if this is not an array.
      do
      {
        m_vertex_input_attribute_descriptions.push_back(vk::VertexInputAttributeDescription{
            .location = location,
            .binding = binding,
            .format = format,
            .offset = offset});
        // update location and offset in case this is an array.
        location += vertex_attribute.basic_type().consumed_locations();
        offset += vertex_attribute.basic_type().array_stride();
      }
      while(--count > 0);
    }
    m_glsl_id_full_to_vertex_attribute_changed = false;
  }
}

void AddVertexShader::register_AddVertexShader_with(task::PipelineFactory* pipeline_factory) const
{
  pipeline_factory->add_to_flat_create_info(&m_vertex_input_binding_descriptions);
  pipeline_factory->add_to_flat_create_info(&m_vertex_input_attribute_descriptions);
}

#ifdef CWDEBUG
void AddVertexShader::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_vertex_input_binding_descriptions:" << m_vertex_input_binding_descriptions <<
      ", m_vertex_input_attribute_descriptions:" << m_vertex_input_attribute_descriptions <<
      ", m_vertex_shader_input_sets:" << m_vertex_shader_input_sets <<
      ", m_vertex_shader_location_context:" << m_vertex_shader_location_context <<
      ", m_glsl_id_full_to_vertex_attribute:" << m_glsl_id_full_to_vertex_attribute;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
