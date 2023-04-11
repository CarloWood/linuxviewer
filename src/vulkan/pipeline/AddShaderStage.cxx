#include "sys.h"
#include "AddShaderStage.h"
#include "SynchronousWindow.h"
#include "shader_builder/ShaderInfo.h"
#include "shader_builder/DeclarationsString.h"
#include "descriptor/CombinedImageSamplerUpdater.h"
#include "utils/malloc_size.h"
#include "debug.h"

namespace vulkan::pipeline {

void AddShaderStage::pre_fill_state()
{
  DoutEntering(dc::vulkan, "AddShaderStage::pre_fill_state() [" << this << "]");
  // build_shaders() must be called every fill_index to re-fill this vector.
  m_shader_stage_create_infos.clear();
}

void AddShaderStage::add_shader(shader_builder::ShaderIndex add_shader_index)
{
  DoutEntering(dc::vulkan, "AddShaderStage::add_shader(add_shader_index) [" << this << "]");
#ifdef CWDEBUG
  shader_builder::ShaderInfo const& add_shader_info = Application::instance().get_shader_info(add_shader_index);
  for (shader_builder::ShaderIndex existing_shader_index : m_shaders_that_need_compiling)
  {
    // Don't add the same shader_index twice.
    ASSERT(add_shader_index != existing_shader_index);
    shader_builder::ShaderInfo const& existing_shader_info = Application::instance().get_shader_info(existing_shader_index);
    // Can't add more than one shader for a given stage.
    ASSERT(add_shader_info.stage() != existing_shader_info.stage());
  }
#endif
  m_shaders_that_need_compiling.push_back(add_shader_index);
}

void AddShaderStage::replace_shader(shader_builder::ShaderIndex remove_shader_index, shader_builder::ShaderIndex add_shader_index)
{
#ifdef CWDEBUG
  shader_builder::ShaderInfo const& remove_shader_info = Application::instance().get_shader_info(remove_shader_index);
  shader_builder::ShaderInfo const& add_shader_info = Application::instance().get_shader_info(add_shader_index);
  // Only use this function to replace a shader of a given stage.
  ASSERT(remove_shader_info.stage() == add_shader_info.stage());
  bool found = false;
#endif
  for (auto shader =  m_shaders_that_need_compiling.begin(); shader != m_shaders_that_need_compiling.end(); ++shader)
  {
    shader_builder::ShaderIndex existing_shader_index = *shader;
    // Don't add the same shader_index twice.
    ASSERT(add_shader_index != existing_shader_index);
    if (existing_shader_index == remove_shader_index)
    {
      *shader = add_shader_index;
      Debug(found = true);
    }
  }
#ifdef CWDEBUG
  // remove_shader_index does not exist.
  ASSERT(found);
#endif
}

vk::ShaderStageFlags AddShaderStage::preprocess_shaders_and_realize_descriptor_set_layouts(task::PipelineFactory* pipeline_factory)
{
  DoutEntering(dc::vulkan, "AddShaderStage::preprocess_shaders_and_realize_descriptor_set_layouts(" << pipeline_factory << ") [" << this << "]");

  vk::ShaderStageFlags preprocessed_stages;

  task::SynchronousWindow const* owning_window = pipeline_factory->owning_window();
  for (shader_builder::ShaderIndex shader_index : m_shaders_that_need_compiling)
  {
    shader_builder::ShaderInfoCache const& shader_info_cache = owning_window->application().get_shader_info(shader_index);
    if (shader_info_cache.is_compiled())
    {
      // Restore additional flat create info vectors from the shader_info_cache, in case we never called preprocess1 even once.
      restore_from_cache(shader_info_cache);
      restore_descriptor_set_layouts(shader_info_cache, pipeline_factory);
      continue;
    }
    // These calls fill PipelineFactory::m_sorted_descriptor_set_layouts with arbitrary binding numbers
    // (in the order that they are found in the shader template code).
    preprocess1(shader_info_cache);
    preprocessed_stages |= shader_info_cache.stage();
  }

  // Realize the descriptor set layouts: if a layout already exists then use the existing
  // handle and update the binding values used in PipelineFactory::m_sorted_descriptor_set_layouts.
  // Otherwise, if it does not already exist, create a new descriptor set layout using the
  // provided binding values as-is.
  pipeline_factory->realize_descriptor_set_layouts({});

  return preprocessed_stages;
}

bool AddShaderStage::build_shaders(
    task::CharacteristicRange* characteristic_range, task::PipelineFactory* pipeline_factory, AIStatefulTask::condition_type locked)
{
  DoutEntering(dc::vulkan, "AddShaderStage::build_shaders(" << pipeline_factory << ") with index = " <<
      m_shaders_that_need_compiling_index << " [" << this << "]");

  task::SynchronousWindow const* owning_window = pipeline_factory->owning_window();
  while (m_shaders_that_need_compiling_index != m_shaders_that_need_compiling.size())
  {
    // If m_shaders_that_need_compiling_index is negative then that means that we received
    // the locked signal for the current m_shaders_that_need_compiling_index and now we have the lock.
    bool have_lock = m_shaders_that_need_compiling_index < 0;
    if (have_lock)
      m_shaders_that_need_compiling_index = -m_shaders_that_need_compiling_index - 1;

    shader_builder::ShaderIndex shader_index = m_shaders_that_need_compiling[m_shaders_that_need_compiling_index];
    shader_builder::ShaderInfoCache& shader_info_cache = Application::instance().get_shader_info(shader_index);

    if (!have_lock &&
        !shader_info_cache.m_task_mutex.lock(characteristic_range, locked))
    {
      // Make the index negative to signal that we're blocking on trying to get the task mutex.
      m_shaders_that_need_compiling_index = -m_shaders_that_need_compiling_index - 1;
      return false;
    }

    {
      statefultask::AdoptLock lock(shader_info_cache.m_task_mutex);

      // Now that we locked shader_info_cache.m_task_mutex we're allowed to access shader_info_cache.
      // The mutex mainly makes sure that only a single task will compile any given shader and assign
      // shader_info_cache.m_shader_module.
      build_shader(owning_window, shader_index, shader_info_cache, m_compiler, m_set_index_hint_map
         COMMA_CWDEBUG_ONLY("AddShaderStage::"));
    }

    // Advance to the next shader, if any.
    ++m_shaders_that_need_compiling_index;
  }

  m_compiler.release();

  // Returns true on success.
  return true;
}

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
    //
    // This is very unexpected; m_shader_variables should not be empty.
    // If it really should be empty then please add a '#version' line at the top of the template code to turn off preprocessing.
    ASSERT(!m_shader_variables.empty());
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
      // Don't call generate_descriptor_set_layout_bindings if the context wasn't changed.
      if (shader_resource_declaration_context->changed_generation() != m_context_changed_generation)
        continue;
      shader_resource_declaration_context->generate_descriptor_set_layout_bindings(shader_info.stage());
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
    shader_builder::ShaderIndex shader_index,
    shader_builder::ShaderInfoCache& shader_info_cache,
    shader_builder::ShaderCompiler const& compiler,
    shader_builder::SPIRVCache& spirv_cache,
    descriptor::SetIndexHintMap const* set_index_hint_map
    COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::build_shader(" << owning_window << ", " << shader_index << " [" << shader_info_cache.name() << "], compiler, spirv_cache, " << vk_utils::print_pointer(set_index_hint_map) << ") [" << this << "]");

  // It should be ok to get a reference to the ShaderInfo element like this,
  // since we could also get it by calling Application::instance().get_shader_info(shader_index).
  shader_builder::ShaderInfo const& shader_info = shader_info_cache;

  if (!shader_info_cache.m_shader_module)
  {
    std::string glsl_source_code_buffer;
    std::string_view glsl_source_code;
    glsl_source_code = preprocess2(shader_info, glsl_source_code_buffer, set_index_hint_map);

    // Add a shader module to this pipeline.
    spirv_cache.compile(glsl_source_code, compiler, shader_info);

    shader_info_cache.m_shader_module = spirv_cache.create_module({}, owning_window->logical_device()
        COMMA_CWDEBUG_ONLY("m_per_stage_shader_module[" + to_string(shader_info.stage()) + "]" + ambifix));

    // Add data to shader_info_cache that might be needed by other pipeline factories
    // because they will no longer call preprocess* after the shader module handle is set.
    update(shader_info_cache);
    cache_descriptor_set_layouts(shader_info_cache);

    // Set an atomic boolean for the sake of optimizing preprocessing away.
    // We can't use m_shader_module for that because preprocess1 is called outside of the
    // critical area of ShaderInfoCache::m_task_mutex. As such there is a race condition,
    // but that will at most to an unnecessary preprocess of the same shader, without
    // compiling it afterwards.
    shader_info_cache.set_compiled();
  }

  m_shader_stage_create_infos.push_back(vk::PipelineShaderStageCreateInfo{
    .flags = vk::PipelineShaderStageCreateFlags(0),
    .stage = shader_info.stage(),
    .module = *shader_info_cache.m_shader_module,
    .pName = "main"
  });
}

void AddShaderStage::cache_descriptor_set_layouts(shader_builder::ShaderInfoCache& shader_info_cache)
{
  declaration_contexts_container_t const& declaration_contexts =
    m_per_stage_declaration_contexts[ShaderStageFlag_to_ShaderStageIndex(shader_info_cache.stage())];

  for (shader_builder::DeclarationContext* declaration_context : declaration_contexts)
  {
    shader_builder::ShaderResourceDeclarationContext* shader_resource_declaration_context =
      dynamic_cast<shader_builder::ShaderResourceDeclarationContext*>(declaration_context);
    if (!shader_resource_declaration_context)   // We're only interested in shader resources here (that have a set index and a binding).
      continue;
    shader_resource_declaration_context->cache_descriptor_set_layouts(shader_info_cache);
  }
}

void AddShaderStage::restore_descriptor_set_layouts(shader_builder::ShaderInfoCache const& shader_info_cache, task::PipelineFactory* pipeline_factory)
{
  DoutEntering(dc::vulkan, "AddShaderStage::restore_descriptor_set_layouts(" << shader_info_cache << ", " << pipeline_factory << ") [" << this << "]");
  Dout(dc::vulkan, "Restoring:");
  vk::ShaderStageFlags shader_stage{shader_info_cache.stage()};
  for (shader_builder::DescriptorSetLayoutBinding const& descriptor_set_layout_binding : shader_info_cache.m_descriptor_set_layouts)
  {
    Dout(dc::vulkan, "--> " << descriptor_set_layout_binding);
    // Does this ever happen??
    ASSERT((descriptor_set_layout_binding.stage_flags() & shader_stage));
    if (!(descriptor_set_layout_binding.stage_flags() & shader_stage))
      continue;
    descriptor::SetIndex set_index = descriptor_set_layout_binding.cached_set_index();
    descriptor::SetIndexHint set_index_hint = m_set_index_hint_map->reverse_convert(set_index);
    descriptor_set_layout_binding.push_back_descriptor_set_layout_binding(pipeline_factory, set_index_hint);
  }
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
void AddShaderStage::prepare_combined_image_sampler_declaration(task::CombinedImageSamplerUpdater const& combined_image_sampler, descriptor::SetIndexHint set_index_hint)
{
  DoutEntering(dc::vulkan|dc::setindexhint, "AddShaderStage::prepare_combined_image_sampler_declaration(" << combined_image_sampler << ", " << set_index_hint << ") [" << this << "]");

  shader_builder::ShaderResourceDeclaration* shader_resource_declaration_ptr = realize_shader_resource_declaration(combined_image_sampler.glsl_id_full(), vk::DescriptorType::eCombinedImageSampler, combined_image_sampler, set_index_hint);
  // CombinedImageSampler only has a single member.
  shader_resource_declaration_ptr->add_member(combined_image_sampler.member());
  // Which is treated here in a general way (but really shader_resource_variables() has just a size of one).
  for (auto& shader_resource_variable : shader_resource_declaration_ptr->shader_resource_variables())
  {
    Dout(dc::vulkan, "Adding " << shader_resource_variable << " to m_shader_variables.");
    m_shader_variables.push_back(&shader_resource_variable);
  }

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
  {
    Dout(dc::vulkan, "Adding " << shader_resource_variable << " to m_shader_variables.");
    m_shader_variables.push_back(&shader_resource_variable);
  }

  // Create and store ShaderResourceDeclarationContext in a map with key set_index_hint, if that doesn't already exists.
  realize_shader_resource_declaration_context(set_index_hint);
}

} // namespace vulkan::pipeline
