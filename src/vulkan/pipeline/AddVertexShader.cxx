#include "sys.h"
#include "AddVertexShader.h"
#include "VertexBuffers.h"
#include "shader_builder/VertexShaderInputSet.h"
#include "vk_utils/type2format.h"
#include <cstring>
#include "debug.h"

namespace vulkan::pipeline {

#if CW_DEBUG
// UnaryPredicate that returns true if the passed shader_variable has a glsl_id_full
// that is equal to the one that this object was constructed with.
struct DebugCompareGlslId
{
  char const* m_glsl_id_full;
  DebugCompareGlslId(char const* glsl_id_full) : m_glsl_id_full(glsl_id_full) { }

  bool operator()(shader_builder::ShaderVariable const* shader_variable)
  {
    return std::strcmp(shader_variable->glsl_id_full(), m_glsl_id_full) == 0;
  }
};
#endif

void AddVertexShader::copy_shader_variables()
{
  DoutEntering(dc::vulkan, "AddVertexShader::copy_shader_variables() [" << this << "]");

  // If we do not have vertex buffers then m_current_vertex_buffers_object will be NULL.
  if (!m_use_vertex_buffers)
    return;

  // Call AddVertexShader::add_vertex_input_bindings with a vulkan::VertexBuffers
  // object from the initialization state of the Characteristic responsible for
  // the vertex shader.
  //
  // If you are not using any VertexBuffers, then set m_use_vertex_buffers to false in the
  // constructor of the pipeline factory characteristic derived from this class.
  ASSERT(m_current_vertex_buffers_object);

  auto const& glsl_id_full_to_vertex_attribute = m_current_vertex_buffers_object->glsl_id_full_to_vertex_attribute();
  for (auto vertex_attribute_iter = glsl_id_full_to_vertex_attribute.begin();
      vertex_attribute_iter != glsl_id_full_to_vertex_attribute.end(); ++vertex_attribute_iter)
  {
    shader_builder::VertexAttribute const& vertex_attribute = vertex_attribute_iter->second;

    // Adding two shader variables with the same glsl_id_full string is not allowed.
    // Did you add the same vertex_attribute twice?
    ASSERT(std::find_if(m_shader_variables.begin(), m_shader_variables.end(), DebugCompareGlslId{vertex_attribute.glsl_id_full()}) ==
        m_shader_variables.end());
    // Add a pointer to all VertexAttribute's in m_glsl_id_full_to_vertex_attribute to m_shader_variables.
    Dout(dc::vulkan, "Adding " << vertex_attribute << " to m_shader_variables.");
    m_shader_variables.push_back(&vertex_attribute);
  }
}

// Clear and then fill m_vertex_input_binding_descriptions and m_vertex_input_attribute_descriptions.
void AddVertexShader::update_vertex_input_descriptions()
{
  DoutEntering(dc::vulkan, "AddVertexShader::update_vertex_input_descriptions() [" << this << "]");

  m_vertex_input_binding_descriptions.clear();
  m_vertex_input_attribute_descriptions.clear();

  // If we didn't add m_vertex_input_binding_descriptions to the flat create info, so it makes no sense to fill them.
  if (!m_use_vertex_buffers)
    return;

  auto const& vertex_shader_input_sets = m_current_vertex_buffers_object->vertex_shader_input_sets();
  for (VertexBufferBindingIndex binding = vertex_shader_input_sets.ibegin(); binding != vertex_shader_input_sets.iend(); ++binding)
  {
    m_vertex_input_binding_descriptions.push_back({
        .binding = static_cast<uint32_t>(binding.get_value()),
        .stride = vertex_shader_input_sets[binding]->chunk_size(),
        .inputRate = vertex_shader_input_sets[binding]->input_rate()});
  }

  auto const& glsl_id_full_to_vertex_attribute = m_current_vertex_buffers_object->glsl_id_full_to_vertex_attribute();
  for (auto vertex_attribute_iter = glsl_id_full_to_vertex_attribute.begin();
      vertex_attribute_iter != glsl_id_full_to_vertex_attribute.end(); ++vertex_attribute_iter)
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
}

void AddVertexShader::register_AddVertexShader_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const
{
  DoutEntering(dc::vulkan, "AddVertexShader::register_AddVertexShader_with(" << pipeline_factory << ", @" << &characteristic_range << ")");
  if (m_use_vertex_buffers)
  {
    pipeline_factory->add_to_flat_create_info(&m_vertex_input_binding_descriptions, characteristic_range);
    pipeline_factory->add_to_flat_create_info(&m_vertex_input_attribute_descriptions, characteristic_range);
  }
}

void AddVertexShader::cache_vertex_input_descriptions(shader_builder::ShaderInfoCache& shader_info_cache)
{
  shader_info_cache.copy(m_vertex_input_binding_descriptions, m_vertex_input_attribute_descriptions);
}

void AddVertexShader::retrieve_vertex_input_descriptions(shader_builder::ShaderInfoCache const& shader_info_cache)
{
  if (m_vertex_input_binding_descriptions.empty() && m_vertex_input_attribute_descriptions.empty())
  {
    m_vertex_input_binding_descriptions = shader_info_cache.m_vertex_input_binding_descriptions;
    m_vertex_input_attribute_descriptions = shader_info_cache.m_vertex_input_attribute_descriptions;
  }
#if CW_DEBUG
  else
  {
    // If we DID already call preprocess1, then we should have gotten the same vertex input descriptions as another pipeline factory of course!
    ASSERT(m_vertex_input_binding_descriptions == shader_info_cache.m_vertex_input_binding_descriptions);
    ASSERT(m_vertex_input_attribute_descriptions == shader_info_cache.m_vertex_input_attribute_descriptions);
  }
#endif
}

void AddVertexShader::add_vertex_input_bindings(VertexBuffers const& vertex_buffers)
{
  DoutEntering(dc::vulkan, "AddVertexShader::add_vertex_input_bindings(" << vertex_buffers << ")");
  m_current_vertex_buffers_object = &vertex_buffers;
}

#ifdef CWDEBUG
void AddVertexShader::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_vertex_input_binding_descriptions:" << m_vertex_input_binding_descriptions <<
      ", m_vertex_input_attribute_descriptions:" << m_vertex_input_attribute_descriptions <<
      ", m_vertex_shader_location_context:" << m_vertex_shader_location_context;
  os << '}';
}
#endif

} // namespace vulkan::pipeline
