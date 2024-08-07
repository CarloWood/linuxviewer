#include "sys.h"
#include "ShaderResourceDeclarationContext.h"
#include "ShaderResourceDeclaration.h"
#include "DeclarationsString.h"
#include "pipeline/PipelineFactory.h"
#include "shader_resource/UniformBuffer.h"
#include "vk_utils/print_flags.h"
#include "vk_utils/snake_case.h"
#include "debug.h"

namespace vulkan::shader_builder {

void ShaderResourceDeclarationContext::update_binding(ShaderResourceDeclaration const* shader_resource_declaration)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "ShaderResourceDeclarationContext::update_binding(@" << *shader_resource_declaration << ") [" << this << "]");
  descriptor::SetIndexHint set_index_hint = shader_resource_declaration->set_index_hint();
  if (m_next_binding.iend() <= set_index_hint)
    m_next_binding.resize(set_index_hint.get_value() + 1);      // New elements are initialized to 0.
  m_bindings[shader_resource_declaration] = m_next_binding[set_index_hint];
  Dout(dc::setindexhint, "Assigned m_bindings[ShaderResourceDeclaration \"" << shader_resource_declaration->glsl_id() << "\"] = " << m_bindings[shader_resource_declaration] << " (m_next_binding[" << set_index_hint << "])");
  shader_resource_declaration->set_binding(m_next_binding[set_index_hint]);
  ++m_next_binding[set_index_hint];
}

void ShaderResourceDeclarationContext::glsl_id_prefix_is_used_in(std::string glsl_id_prefix, vk::ShaderStageFlagBits shader_stage, ShaderResourceDeclaration const* shader_resource_declaration, int context_changed_generation)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::glsl_id_prefix_is_used_in with shader_resource_declaration(\"" <<
      glsl_id_prefix << "\", " << shader_stage << ", " << shader_resource_declaration << ", " << context_changed_generation << ")");

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
      // See https://www.reddit.com/r/vulkan/comments/4gvmus/comment/d2lbwsm/?utm_source=share&utm_medium=web2x&context=3
      ASSERT(false);
  }

  // Mark that it must be updated.
  m_changed_generation = context_changed_generation;
}

// Called from AddShaderStage::preprocess1.
void ShaderResourceDeclarationContext::generate_descriptor_set_layout_bindings(vk::ShaderStageFlagBits shader_stage)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "ShaderResourceDeclarationContext::generate_descriptor_set_layout_bindings(" << shader_stage << ") [" << this << "]");

  Dout(dc::vulkan, "m_bindings contains:");
  for (auto&& shader_resource_binding_pair : m_bindings)
  {
    ShaderResourceDeclaration const* shader_resource_declaration = shader_resource_binding_pair.first;
    Dout(dc::vulkan, "--> " << *shader_resource_declaration);
    // Paranoia check: these should be equal.
    ASSERT(shader_resource_declaration->binding() == shader_resource_binding_pair.second);
    if (!(shader_resource_declaration->stage_flags() & shader_stage))
      continue;
    descriptor::SetIndexHint set_index_hint = shader_resource_declaration->set_index_hint();
    shader_resource_declaration->push_back_descriptor_set_layout_binding(m_owning_factory, set_index_hint);
  }
}

void ShaderResourceDeclarationContext::add_declarations_for_stage(DeclarationsString& declarations_out, vk::ShaderStageFlagBits shader_stage) const
{
  DoutEntering(dc::vulkan|dc::setindexhint, "ShaderResourceDeclarationContext::generate(declarations_out, " << shader_stage << ") [" << this << "]");

  std::ostringstream oss;
  Dout(dc::vulkan, "Generating declaration; running over m_bindings that has the contents: " << m_bindings);
  for (auto&& shader_resource_binding_pair : m_bindings)
  {
    Dout(dc::vulkan, "shader_resource_binding_pair = { &" << *shader_resource_binding_pair.first << ", " << shader_resource_binding_pair.second << " }");
    ShaderResourceDeclaration const* shader_resource_declaration = shader_resource_binding_pair.first;
    if (!(shader_resource_declaration->stage_flags() & shader_stage))
      continue;
    descriptor::SetIndex set_index = m_set_index_hint_map3->convert(shader_resource_declaration->set_index_hint());
    uint32_t binding = shader_resource_binding_pair.second;

    switch (shader_resource_declaration->descriptor_type())
    {
      case vk::DescriptorType::eCombinedImageSampler:
      {
        auto const& variable = shader_resource_declaration->shader_resource_variables();
        // A texture only has one "member".
        ASSERT(variable.size() == 1);
        ShaderResourceVariable const& shader_variable = *variable.begin();
        // layout(set = 0, binding = 0) uniform sampler2D u_Texture_background;
        oss << "layout(set = " << set_index.get_value() << ", binding = " << binding << ") uniform sampler2D " << shader_variable.name();
        int32_t descriptor_array_size = shader_resource_declaration->descriptor_array_size();
        if (descriptor_array_size > 1)
          oss << "[" << descriptor_array_size << "]";
        else if (descriptor_array_size < 0)
          oss << "[]";
        oss << ";\t// " << shader_variable.glsl_id_full();
        oss << "\n";
        break;
      }
      case vk::DescriptorType::eUniformBuffer:
      {
        // struct TopPosition {
        //   float unused1;
        //   vec3 v[7];
        // };
        //
        // layout(set = 0, binding = 1) uniform u_s0b1 {
        //   float unused;
        //   TopPosition foo;
        // } MyUniformBuffer;

        oss << "layout(set = " << set_index.get_value() << ", binding = " << binding << ") uniform "
          "u_s" << set_index.get_value() << "b" << binding << " { // " << shader_resource_declaration->glsl_id() << "\n";

        std::string const& prefix = shader_resource_declaration->glsl_id();
        ShaderResourceBase const& base = shader_resource_declaration->shader_resource();
        shader_builder::UniformBufferBase const& uniform_buffer = static_cast<shader_builder::UniformBufferBase const&>(base);
        auto const& members = uniform_buffer.members();
        declarations_out.write_members_to(oss, members);
        oss << "} " << prefix << ";\n";
        break;
      }
      default:
        //FIXME: not implemented.
        ASSERT(false);
    }
  }
  declarations_out += oss.str();
}

void ShaderResourceDeclarationContext::cache_descriptor_set_layouts(ShaderInfoCache& shader_info_cache)
{
  DoutEntering(dc::vulkan, "ShaderResourceDeclarationContext::cache_descriptor_set_layouts(" << shader_info_cache << ")");
  vk::ShaderStageFlagBits shader_stage = shader_info_cache.stage();
  for (auto&& shader_resource_binding_pair : m_bindings)
  {
    ShaderResourceDeclaration const* shader_resource_declaration = shader_resource_binding_pair.first;
    if (!(shader_resource_declaration->stage_flags() & shader_stage))
      continue;
    shader_info_cache.m_descriptor_set_layouts.push_back(*shader_resource_declaration);
    // The SetIndexHint can be different between pipeline factories, therefore, store the SetIndex.
    descriptor::SetIndex set_index = m_set_index_hint_map3->convert(shader_resource_declaration->set_index_hint());
    shader_info_cache.m_descriptor_set_layouts.back().set_set_index(set_index);
    Dout(dc::vulkan, "Cached " << shader_info_cache.m_descriptor_set_layouts.back());
  }
}

#ifdef CWDEBUG
void ShaderResourceDeclarationContext::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_next_binding:" << m_next_binding <<
      ", m_bindings:" << m_bindings <<
      ", m_owning_factory:" << m_owning_factory;
  os << '}';
}
#endif

} // namespace vulkan::shader_builder
