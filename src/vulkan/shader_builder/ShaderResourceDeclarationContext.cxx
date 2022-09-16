#include "sys.h"
#include "ShaderResourceDeclarationContext.h"
#include "ShaderResourceDeclaration.h"
#include "shader_resource/UniformBuffer.h"
#include "pipeline/ShaderInputData.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/snake_case.h"
#include "debug.h"

namespace vulkan::shader_builder {

#if 0
void ShaderResourceDeclarationContext::reserve_binding(descriptor::SetKey set_key)
{
  descriptor::SetIndex set_index = m_owning_shader_input_data->get_set_index(set_key);
  if (m_next_binding.iend() <= set_index)
    m_next_binding.resize(set_index.get_value() + 1);           // New elements are initialized to 0.
  m_next_binding[set_index] += 1;
}
#endif

void ShaderResourceDeclarationContext::update_binding(ShaderResourceDeclaration const* shader_resource_declaration)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::update_binding(@" << *shader_resource_declaration << ") [" << this << "]");
  descriptor::SetIndexHint set_index_hint = shader_resource_declaration->set_index_hint();
  if (m_next_binding.iend() <= set_index_hint)
    m_next_binding.resize(set_index_hint.get_value() + 1);      // New elements are initialized to 0.
  m_bindings[shader_resource_declaration] = m_next_binding[set_index_hint];
  int number_of_bindings = 1;
#if 0
  if (vertex_attribute->layout().m_array_size > 0)
    number_of_attribute_indices *= vertex_attribute->layout().m_array_size;
#endif
  Dout(dc::notice, "Changing m_next_binding[set_index_hint:" << set_index_hint << "] from " << m_next_binding[set_index_hint] << " to " << (m_next_binding[set_index_hint] + number_of_bindings) << ".");
  m_next_binding[set_index_hint] += number_of_bindings;
}

void ShaderResourceDeclarationContext::glsl_id_prefix_is_used_in(std::string glsl_id_prefix, vk::ShaderStageFlagBits shader_stage, ShaderResourceDeclaration const* shader_resource_declaration, pipeline::ShaderInputData* shader_input_data)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::glsl_id_prefix_is_used_in with shader_resource_declaration(\"" <<
      glsl_id_prefix << "\", " << shader_stage << ", " << shader_resource_declaration << ", " << shader_input_data << ")");

  // Only register one binding per shader resource per set.
  if (m_bindings.find(shader_resource_declaration) != m_bindings.end())
    return;

  switch (shader_resource_declaration->descriptor_type())
  {
    case vk::DescriptorType::eCombinedImageSampler:
    case vk::DescriptorType::eUniformBuffer:
      //FIXME: Is this correct? Can't I use this for every type?
      update_binding(shader_resource_declaration);
      break;
    default:
      //FIXME: implement
      ASSERT(false);
  }
}

std::string ShaderResourceDeclarationContext::generate(vk::ShaderStageFlagBits shader_stage) const
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::generate(" << shader_stage << ") [" << this << "]");
  Dout(dc::vulkan, "Generating declaration; running over m_bindings that has the contents: " << m_bindings);

  std::ostringstream oss;
  for (auto&& shader_resource_binding_pair : m_bindings)
  {
    Dout(dc::vulkan, "shader_resource_binding_pair = { &" <<
        *shader_resource_binding_pair.first << ", " << shader_resource_binding_pair.second << " }");
    ShaderResourceDeclaration const* shader_resource_declaration = shader_resource_binding_pair.first;
    if (!(shader_resource_declaration->stage_flags() & shader_stage))
      continue;
    uint32_t binding = shader_resource_binding_pair.second;
    m_owning_shader_input_data->push_back_descriptor_set_layout_binding(shader_resource_declaration->set_index_hint(), {
        .binding = binding,
        .descriptorType = shader_resource_declaration->descriptor_type(),
        .descriptorCount = 1,
        .stageFlags = shader_resource_declaration->stage_flags(),
        .pImmutableSamplers = nullptr
    });

    switch (shader_resource_declaration->descriptor_type())
    {
      case vk::DescriptorType::eCombinedImageSampler:
      {
        auto const& variable = shader_resource_declaration->shader_resource_variables();
        // A texture only has one "member".
        ASSERT(variable.size() == 1);
        ShaderResourceVariable const& shader_variable = *variable.begin();
        // layout(set = 0, binding = 0) uniform sampler2D u_Texture_background;
        oss << "layout(set = " << shader_resource_declaration->set_index_hint().get_value() << ", binding = " << binding << ") uniform sampler2D " << shader_variable.name();
#if 0
        if (layout.m_array_size > 0)
          oss << '[' << layout.m_array_size << ']';
#endif
        oss << ";\t// " << shader_variable.glsl_id_full() << "\n";
        break;
      }
      case vk::DescriptorType::eUniformBuffer:
      {
        // struct TopPosition {
        //   mat2 unused1;
        //   float x;
        // };
        //
        // layout(set = 0, binding = 1) uniform u_s0b1 {
        //   TopPosition m_top_position;
        // } v4238767198234540653;

        std::string const& prefix = shader_resource_declaration->glsl_id();
        std::size_t const prefix_hash = std::hash<std::string>{}(prefix);
        shader_resource::Base const& base = shader_resource_declaration->shader_resource();
        shader_resource::UniformBufferBase const& uniform_buffer = static_cast<shader_resource::UniformBufferBase const&>(base);
        auto const& members = uniform_buffer.members();
        oss << "struct " << prefix << " {\n";
        for (ShaderResourceMember const& member : members)
        {
          BasicType basic_type = member.basic_type();
          oss << "  " << glsl::type2name(basic_type.scalar_type(), basic_type.rows(), basic_type.cols()) << ' ' << member.member_name() << ";\n";
        }
        oss << "};\nlayout(set = " << shader_resource_declaration->set_index_hint().get_value() << ", binding = " << binding << ") uniform "
          "u_s" << shader_resource_declaration->set_index_hint().get_value() << "b" << binding << " {\n  " <<
          prefix << " m_" << vk_utils::snake_case(prefix) << ";\n} v" << prefix_hash << ";\n";
        break;
      }
      default:
        //FIXME: not implemented.
        ASSERT(false);
    }
  }
  return oss.str();
}

#ifdef CWDEBUG
void ShaderResourceDeclarationContext::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_next_binding:" << m_next_binding <<
      ", m_bindings:" << m_bindings <<
      ", m_owning_shader_input_data:" << m_owning_shader_input_data;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
