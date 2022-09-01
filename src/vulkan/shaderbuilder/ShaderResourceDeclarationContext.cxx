#include "sys.h"
#include "ShaderResourceDeclarationContext.h"
#include "ShaderResource.h"
#include "vk_utils/print_flags.h"
#include "debug.h"

namespace vulkan::shaderbuilder {

void ShaderResourceDeclarationContext::update_binding(ShaderResource const* shader_resource)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::update_binding(@" << *shader_resource << ") [" << this << "]");
  m_bindings[shader_resource] = m_next_binding;
  int number_of_bindings = 1;
#if 0
  if (vertex_attribute->layout().m_array_size > 0)
    number_of_attribute_indices *= vertex_attribute->layout().m_array_size;
#endif
  Dout(dc::notice, "Changing m_next_binding from " << m_next_binding << " to " << (m_next_binding + number_of_bindings) << ".");
  m_next_binding += number_of_bindings;
}

void ShaderResourceDeclarationContext::glsl_id_str_is_used_in(char const* glsl_id_str, vk::ShaderStageFlagBits shader_stage, ShaderVariable const* shader_variable, pipeline::ShaderInputData* shader_input_data)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::glsl_id_str_is_used_in with shader_resource(" <<
      NAMESPACE_DEBUG::print_string(glsl_id_str) << ", " << shader_stage << ", " << shader_variable << ", " << shader_input_data << ")");

  ShaderResource const* shader_resource = static_cast<ShaderResource const*>(shader_variable);
  Dout(dc::vulkan, "shader_resource = " << *shader_resource);

  switch (shader_resource->descriptor_type())
  {
    case vk::DescriptorType::eCombinedImageSampler:
      //FIXME: Is this correct? Can't I use this for every type?
      update_binding(shader_resource);
      break;
    default:
      //FIXME: implement
      ASSERT(false);
  }
}

std::string ShaderResourceDeclarationContext::generate_declaration(vk::ShaderStageFlagBits shader_stage) const
{
  std::ostringstream oss;
  ASSERT(m_next_binding <= 999); // 3 chars max.
  for (auto&& shader_resource_binding_pair : m_bindings)
  {
    ShaderResource const* shader_resource = shader_resource_binding_pair.first;
    ShaderVariable const* shader_variable = shader_resource;
    uint32_t binding = shader_resource_binding_pair.second;

    switch (shader_resource->descriptor_type())
    {
      case vk::DescriptorType::eCombinedImageSampler:
        // layout(set=0, binding=0) uniform sampler2D u_Texture_background;
        oss << "layout(set = " << shader_resource->set() << ", binding=" << binding << ") uniform sampler2D " << shader_variable->name();
#if 0
        if (layout.m_array_size > 0)
          oss << '[' << layout.m_array_size << ']';
#endif
        oss << ";\t// " << shader_resource->glsl_id_str() << "\n";
        break;
      default:
        //FIXME: not implemented.
        ASSERT(false);
    }
  }
  return oss.str();
}

} // namespace vulkan::shaderbuilder
