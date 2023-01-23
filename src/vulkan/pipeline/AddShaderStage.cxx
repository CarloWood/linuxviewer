#include "sys.h"
#include "AddShaderStage.h"
#include "SynchronousWindow.h"
#include "shader_builder/ShaderInfo.h"
#include "shader_builder/DeclarationsString.h"
#include "descriptor/CombinedImageSamplerUpdater.h"
#include "utils/malloc_size.h"
#include "debug.h"

namespace vulkan::pipeline {

void AddShaderStage::preprocess1(shader_builder::ShaderInfo const& shader_info)
{
  DoutEntering(dc::vulkan, "AddShaderStage::preprocess1(" << shader_info << ") [" << this << "]");

  std::string_view const source = shader_info.glsl_template_code();

  // Assume no preprocessing is necessary if the source already starts with "#version".
  if (!source.starts_with("#version"))
  {
    // Increment the context generation.
    ++m_context_changed_generation;

    declaration_contexts_container_t& declaration_contexts = m_per_stage_declaration_contexts[ShaderStageFlag_to_ShaderStageIndex(shader_info.stage())];
    // m_shader_variables contains a number of strings that we need to find in the source.
    // They may occur zero or more times.
    for (shader_builder::ShaderVariable const* shader_variable : m_shader_variables)
    {
      std::string match_string = shader_variable->glsl_id_full();
      Dout(dc::notice|continued_cf, "Looking for \"" << match_string << "\"... ");
      if (source.find(match_string) != std::string_view::npos)
      {
        Dout(dc::finish, "(found)");
        declaration_contexts.insert(shader_variable->is_used_in(shader_info.stage(), this));
      }
      else
        Dout(dc::finish, "(not found)");
    }
    for (shader_builder::DeclarationContext* declaration_context : declaration_contexts)
    {
      shader_builder::ShaderResourceDeclarationContext* shader_resource_declaration_context =
        dynamic_cast<shader_builder::ShaderResourceDeclarationContext*>(declaration_context);
      if (!shader_resource_declaration_context)   // We're only interested in shader resources here (that have a set index and a binding).
        continue;
      // Don't call generate1 if the context wasn't changed.
      if (shader_resource_declaration_context->changed_generation() != m_context_changed_generation)
        continue;
      shader_resource_declaration_context->generate1(shader_info.stage());
    }
  }
}

// Called from build_shader.
std::string_view AddShaderStage::preprocess2(
    shader_builder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer, descriptor::SetIndexHintMap const* set_index_hint_map) const
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::preprocess2(" << shader_info << ", glsl_source_code_buffer, " << vk_utils::print_pointer(set_index_hint_map) << ") [" << this << "]");

  std::string_view const source = shader_info.glsl_template_code();

  // Assume no preprocessing is necessary if the source already starts with "#version".
  if (source.starts_with("#version"))
    return source;

  declaration_contexts_container_t const& declaration_contexts = m_per_stage_declaration_contexts[ShaderStageFlag_to_ShaderStageIndex(shader_info.stage())];

  for (shader_builder::DeclarationContext* declaration_context : declaration_contexts)
  {
    shader_builder::ShaderResourceDeclarationContext* shader_resource_declaration_context =
      dynamic_cast<shader_builder::ShaderResourceDeclarationContext*>(declaration_context);
    if (!shader_resource_declaration_context)   // We're only interested in shader resources here (that have a set index and a binding).
      continue;
    shader_resource_declaration_context->set_set_index_hint_map(set_index_hint_map);
  }

  // Generate the declarations.
  shader_builder::DeclarationsString declarations;
  for (shader_builder::DeclarationContext const* declaration_context : declaration_contexts)
    declaration_context->add_declarations_for_stage(declarations, shader_info.stage());
  declarations.add_newline();   // For pretty printing debug output.

  // Store each position where a match string occurs, together with an std::pair
  // containing the found substring that has to be replaced (first) and the
  // string that it has to be replaced with (second).
  std::map<size_t, std::pair<std::string, std::string>> positions;

  // m_shader_variables contains a number of strings that we need to find in the source. They may occur zero or more times.
  int id_to_name_growth = 0;
  Dout(dc::vulkan, "Finding substitutions:");
  for (shader_builder::ShaderVariable const* shader_variable : m_shader_variables)
  {
    std::string match_string = shader_variable->glsl_id_full();
    Dout(dc::vulkan, "  Trying \"" << match_string << "\".");
    for (size_t pos = 0; (pos = source.find(match_string, pos)) != std::string_view::npos; pos += match_string.length())
    {
      id_to_name_growth += shader_variable->name().length() - match_string.length();
      Dout(dc::vulkan, "Found! Adding substitution: " << match_string << " --> " << shader_variable->substitution());
      positions[pos] = std::make_pair(match_string, shader_variable->substitution());
    }
  }
  Dout(dc::vulkan, "Done finding substitutions.");

  static constexpr char const* version_header = "#version 450\n\n";
  size_t final_source_code_size = std::strlen(version_header) + declarations.length() + source.length() + id_to_name_growth;

  glsl_source_code_buffer.reserve(utils::malloc_size(final_source_code_size + 1) - 1);
  glsl_source_code_buffer = version_header;
  glsl_source_code_buffer += declarations.content();

  // Next copy alternating, the characters in between the strings and the replacements of the substrings.
  size_t start = 0;
  for (auto&& p : positions)
  {
    // Copy the characters leading up to the string at position p.
    glsl_source_code_buffer += source.substr(start, p.first - start);
    // Advance start to just after the found string.
    start = p.first + p.second.first.length();
    // Append the replacement string.
    glsl_source_code_buffer += p.second.second;
  }
  // Copy the remaining characters.
  glsl_source_code_buffer += source.substr(start);
  glsl_source_code_buffer.shrink_to_fit();
  return glsl_source_code_buffer;
}

void AddShaderStage::build_shader(task::SynchronousWindow const* owning_window,
    shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
    shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::build_shader(" << owning_window << ", " << shader_index << ", compiler, spirv_cache, " << vk_utils::print_pointer(set_index_hint_map) << ") [" << this << "]");

  std::string glsl_source_code_buffer;
  std::string_view glsl_source_code;
  shader_builder::ShaderInfo const& shader_info = owning_window->application().get_shader_info(shader_index);
  glsl_source_code = preprocess2(shader_info, glsl_source_code_buffer, set_index_hint_map);

  // Add a shader module to this pipeline.
  spirv_cache.compile(glsl_source_code, compiler, shader_info);

  vk::UniqueShaderModule& shader_module_ptr = m_per_stage_shader_module[ShaderStageFlag_to_ShaderStageIndex(shader_info.stage())];
  // Paranoia check. If the AddShaderStage virtual base class is used more than once then
  // the m_shader_module handle should be moved to a safe place before overwriting it!
  ASSERT(!shader_module_ptr);
  shader_module_ptr = spirv_cache.create_module({}, owning_window->logical_device()
      COMMA_CWDEBUG_ONLY(".m_shader_module[" + to_string(shader_info.stage()) + "]" + ambifix));
  m_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo{
    .flags = vk::PipelineShaderStageCreateFlags(0),
    .stage = shader_info.stage(),
    .module = *shader_module_ptr,
    .pName = "main"
  });
}

// Called from prepare_combined_image_sampler_declaration and prepare_uniform_buffer_declaration.
void AddShaderStage::realize_shader_resource_declaration_context(descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::realize_shader_resource_declaration_context(" << set_index_hint << ") [" << this << "]");
  // Add a ShaderResourceDeclarationContext with key set_index_hint, if that doesn't already exists.
  if (m_set_index_hint_to_shader_resource_declaration_context.find(set_index_hint) == m_set_index_hint_to_shader_resource_declaration_context.end())
  {
    DEBUG_ONLY(auto res2 =)
      m_set_index_hint_to_shader_resource_declaration_context.try_emplace(set_index_hint, get_owning_factory());
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
    Dout(dc::setindexhint, "Could not find " << set_index_hint << " in m_set_index_hint_to_shader_resource_declaration_context; added new ShaderResourceDeclarationContext @" << (void*)&*res2.first << ".");
  }
}

// Called from CombinedImageSampler::prepare_shader_resource_declaration,
// which is the override of shader_builder::ShaderResourceBase that is
// called from prepare_shader_resource_declarations.
//
// This function is called once for each combined_image_sampler that was passed to a call to add_combined_image_sampler.
void AddShaderStage::prepare_combined_image_sampler_declaration(descriptor::CombinedImageSamplerUpdater const& combined_image_sampler, descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::prepare_combined_image_sampler_declaration(" << combined_image_sampler << ", " << set_index_hint << ") [" << this << "]");

  shader_builder::ShaderResourceDeclaration* shader_resource_declaration_ptr = realize_shader_resource_declaration(combined_image_sampler.glsl_id_full(), vk::DescriptorType::eCombinedImageSampler, combined_image_sampler, set_index_hint);
  // CombinedImageSampler only has a single member.
  shader_resource_declaration_ptr->add_member(combined_image_sampler.member());
  // Which is treated here in a general way (but really shader_resource_variables() has just a size of one).
  for (auto& shader_resource_variable : shader_resource_declaration_ptr->shader_resource_variables())
    m_shader_variables.push_back(&shader_resource_variable);

#if 1
  Dout(dc::debug, "m_shader_variables (@" << (void*)&m_shader_variables << " [" << this << "]) currently contains:");
  for (auto shader_variable_ptr : m_shader_variables)
    Dout(dc::debug, "  " << vk_utils::print_pointer(shader_variable_ptr));
#endif

  // Create and store ShaderResourceDeclarationContext in a map with key set_index_hint, if that doesn't already exists.
  realize_shader_resource_declaration_context(set_index_hint);
}

// Called from UniformBufferBase::prepare_shader_resource_declaration,
// which is the override of shader_builder::ShaderResourceBase that is
// called from prepare_shader_resource_declarations.
//
// This function is called once for each uniform_buffer that was passed to a call to add_uniform_buffer.
void AddShaderStage::prepare_uniform_buffer_declaration(shader_builder::UniformBufferBase const& uniform_buffer, descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::prepare_uniform_buffer_declaration(" << uniform_buffer << ", " << set_index_hint << ") [" << this << "]");

  shader_builder::ShaderResourceDeclaration* shader_resource_ptr = realize_shader_resource_declaration(uniform_buffer.glsl_id(), vk::DescriptorType::eUniformBuffer, uniform_buffer, set_index_hint);
  shader_resource_ptr->add_members(uniform_buffer.members());
  for (auto const& shader_resource_variable : shader_resource_ptr->shader_resource_variables())
    m_shader_variables.push_back(&shader_resource_variable);

  // Create and store ShaderResourceDeclarationContext in a map with key set_index_hint, if that doesn't already exists.
  realize_shader_resource_declaration_context(set_index_hint);
}

} // namespace vulkan::pipeline
