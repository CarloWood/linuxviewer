#include "sys.h"
#include "ShaderInputData.h"
#include "SynchronousWindow.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "shader_builder/ShaderResourceDeclarationContext.h"
#include "shader_builder/DeclarationsString.h"
#include "utils/malloc_size.h"
#include "debug.h"
#include <cstring>

namespace vulkan::pipeline {
#ifdef CWDEBUG
using NAMESPACE_DEBUG::print_string;
#endif

// Returns the declaration contexts that are used in this shader.
void ShaderInputData::preprocess1(shader_builder::ShaderInfo const& shader_info)
{
  DoutEntering(dc::vulkan, "ShaderInputData::preprocess1(" << shader_info << ") [" << this << "]");

  declaration_contexts_container_t& declaration_contexts = m_per_stage_declaration_contexts[shader_info.stage()]; // All the declaration contexts that are involved.
  std::string_view const source = shader_info.glsl_template_code();

  // Assume no preprocessing is necessary if the source already starts with "#version".
  if (!source.starts_with("#version"))
  {
    // m_shader_variables contains a number of strings that we need to find in the source.
    // They may occur zero or more times.
    Dout(dc::always, "Searching in m_shader_variables @" << (void*)&m_shader_variables << " [" << this << "].");
    for (shader_builder::ShaderVariable const* shader_variable : m_shader_variables)
    {
      std::string match_string = shader_variable->glsl_id_full();
      if (source.find(match_string) != std::string_view::npos)
        declaration_contexts.insert(shader_variable->is_used_in(shader_info.stage(), this));
    }
    for (shader_builder::DeclarationContext const* declaration_context : declaration_contexts)
    {
      shader_builder::ShaderResourceDeclarationContext const* shader_resource_declaration_context =
        dynamic_cast<shader_builder::ShaderResourceDeclarationContext const*>(declaration_context);
      if (!shader_resource_declaration_context)   // We're only interested in shader resources here (that have a set index and a binding).
        continue;
      shader_resource_declaration_context->generate1(shader_info.stage());
    }
  }
}

std::string_view ShaderInputData::preprocess2(shader_builder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer, descriptor::SetBindingMap const& set_binding_map) const
{
  DoutEntering(dc::vulkan, "ShaderInputData::preprocess2(" << shader_info << ", glsl_source_code_buffer) [" << this << "]");

  declaration_contexts_container_t const& declaration_contexts = m_per_stage_declaration_contexts.at(shader_info.stage()); // All the declaration contexts that are involved.
  std::string_view const source = shader_info.glsl_template_code();

  // Assume no preprocessing is necessary if the source already starts with "#version".
  if (source.starts_with("#version"))
    return source;

  for (shader_builder::DeclarationContext* declaration_context : declaration_contexts)
  {
    shader_builder::ShaderResourceDeclarationContext* shader_resource_declaration_context =
      dynamic_cast<shader_builder::ShaderResourceDeclarationContext*>(declaration_context);
    if (!shader_resource_declaration_context)   // We're only interested in shader resources here (that have a set index and a binding).
      continue;
    shader_resource_declaration_context->set_set_binding_map(&set_binding_map);
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
  for (shader_builder::ShaderVariable const* shader_variable : m_shader_variables)
  {
    std::string match_string = shader_variable->glsl_id_full();
    for (size_t pos = 0; (pos = source.find(match_string, pos)) != std::string_view::npos; pos += match_string.length())
    {
      id_to_name_growth += shader_variable->name().length() - match_string.length();
      positions[pos] = std::make_pair(match_string, shader_variable->substitution());
    }
  }

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

std::vector<vk::VertexInputBindingDescription> ShaderInputData::vertex_binding_descriptions() const
{
  DoutEntering(dc::vulkan, "ShaderInputData::vertex_binding_descriptions() [" << this << "]");
  std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions;
  uint32_t binding = 0;
  for (auto const* vextex_input_set : m_vertex_shader_input_sets)
  {
    vertex_binding_descriptions.push_back({
        .binding = binding,
        .stride = vextex_input_set->chunk_size(),
        .inputRate = vextex_input_set->input_rate()});
    ++binding;
  }
  return vertex_binding_descriptions;
}

namespace {

// The following format must be supported by Vulkan (so no test is necessary):
//
// VK_FORMAT_R8_UNORM
// VK_FORMAT_R8_SNORM
// VK_FORMAT_R8_UINT
// VK_FORMAT_R8_SINT
// VK_FORMAT_R8G8_UNORM
// VK_FORMAT_R8G8_SNORM
// VK_FORMAT_R8G8_UINT
// VK_FORMAT_R8G8_SINT
// VK_FORMAT_R8G8B8A8_UNORM
// VK_FORMAT_R8G8B8A8_SNORM
// VK_FORMAT_R8G8B8A8_UINT
// VK_FORMAT_R8G8B8A8_SINT
// VK_FORMAT_B8G8R8A8_UNORM
// VK_FORMAT_A8B8G8R8_UNORM_PACK32
// VK_FORMAT_A8B8G8R8_SNORM_PACK32
// VK_FORMAT_A8B8G8R8_UINT_PACK32
// VK_FORMAT_A8B8G8R8_SINT_PACK32
// VK_FORMAT_A2B10G10R10_UNORM_PACK32
// VK_FORMAT_R16_UNORM
// VK_FORMAT_R16_SNORM
// VK_FORMAT_R16_UINT
// VK_FORMAT_R16_SINT
// VK_FORMAT_R16_SFLOAT
// VK_FORMAT_R16G16_UNORM
// VK_FORMAT_R16G16_SNORM
// VK_FORMAT_R16G16_UINT
// VK_FORMAT_R16G16_SINT
// VK_FORMAT_R16G16_SFLOAT
// VK_FORMAT_R16G16B16A16_UNORM
// VK_FORMAT_R16G16B16A16_SNORM
// VK_FORMAT_R16G16B16A16_UINT
// VK_FORMAT_R16G16B16A16_SINT
// VK_FORMAT_R16G16B16A16_SFLOAT
// VK_FORMAT_R32_UINT
// VK_FORMAT_R32_SINT
// VK_FORMAT_R32_SFLOAT
// VK_FORMAT_R32G32_UINT
// VK_FORMAT_R32G32_SINT
// VK_FORMAT_R32G32_SFLOAT
// VK_FORMAT_R32G32B32_UINT
// VK_FORMAT_R32G32B32_SINT
// VK_FORMAT_R32G32B32_SFLOAT
// VK_FORMAT_R32G32B32A32_UINT
// VK_FORMAT_R32G32B32A32_SINT
// VK_FORMAT_R32G32B32A32_SFLOAT

vk::Format type2format(shader_builder::BasicType glsl_type)
{
  vk::Format format;
  int rows = glsl_type.rows();
  glsl::ScalarIndex scalar_type = glsl_type.scalar_type();
  switch (scalar_type)
  {
    case glsl::eFloat:
      // 32_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sfloat;
          break;
        case 2:
          format = vk::Format::eR32G32Sfloat;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sfloat;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sfloat;
          break;
      }
      break;
    case glsl::eDouble:
      // 64_SFLOAT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR64Sfloat;
          break;
        case 2:
          format = vk::Format::eR64G64Sfloat;
          break;
        case 3:
          format = vk::Format::eR64G64B64Sfloat;
          break;
        case 4:
          format = vk::Format::eR64G64B64A64Sfloat;
          break;
      }
      break;
    case glsl::eBool:
      // 8_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Uint;
          break;
        case 2:
          format = vk::Format::eR8G8Uint;
          break;
        case 3:
          format = vk::Format::eR8G8B8Uint;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Uint;
          break;
      }
      break;
    case glsl::eInt:
      // 32_SINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Sint;
          break;
        case 2:
          format = vk::Format::eR32G32Sint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Sint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Sint;
          break;
      }
      break;
    case glsl::eUint:
      // 32_UINT
      switch (rows)
      {
        case 1:
          format = vk::Format::eR32Uint;
          break;
        case 2:
          format = vk::Format::eR32G32Uint;
          break;
        case 3:
          format = vk::Format::eR32G32B32Uint;
          break;
        case 4:
          format = vk::Format::eR32G32B32A32Uint;
          break;
      }
      break;
    case glsl::eInt8:
      // int8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Snorm;
          break;
        case 2:
          format = vk::Format::eR8G8Snorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Snorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Snorm;
          break;
      }
      break;
    case glsl::eUint8:
      // uint8_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR8Unorm;
          break;
        case 2:
          format = vk::Format::eR8G8Unorm;
          break;
        case 3:
          format = vk::Format::eR8G8B8Unorm;
          break;
        case 4:
          format = vk::Format::eR8G8B8A8Unorm;
          break;
      }
      break;
    case glsl::eInt16:
      // int16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Snorm;
          break;
        case 2:
          format = vk::Format::eR16G16Snorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Snorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Snorm;
          break;
      }
      break;
    case glsl::eUint16:
      // uint16_t
      switch (rows)
      {
        case 1:
          format = vk::Format::eR16Unorm;
          break;
        case 2:
          format = vk::Format::eR16G16Unorm;
          break;
        case 3:
          format = vk::Format::eR16G16B16Unorm;
          break;
        case 4:
          format = vk::Format::eR16G16B16A16Unorm;
          break;
      }
      break;
  }
  return format;
}

} // namespace

std::vector<vk::VertexInputAttributeDescription> ShaderInputData::vertex_input_attribute_descriptions() const
{
  DoutEntering(dc::vulkan, "ShaderInputData::vertex_input_attribute_descriptions() [" << this << "]");

  std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
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
    // in turn called by ShaderInputData::preprocess.
    ASSERT(iter != m_vertex_shader_location_context.locations().end());
    uint32_t location = iter->second;

    uint32_t const binding = static_cast<uint32_t>(vertex_attribute.binding().get_value());
    vk::Format const format = type2format(vertex_attribute.basic_type());
    uint32_t offset = vertex_attribute.offset();
    int count = vertex_attribute.array_size();          // Zero if this is not an array.
    do
    {
      vertex_input_attribute_descriptions.push_back(vk::VertexInputAttributeDescription{
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
  return vertex_input_attribute_descriptions;
}

void ShaderInputData::add_texture(shader_builder::shader_resource::Texture const& texture,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "ShaderInputData::add_texture(" << texture << ", " << preferred_descriptor_sets << ", " << undesirable_descriptor_sets << ")");

  //FIXME: implement using preferred_descriptor_sets / undesirable_descriptor_sets.
  descriptor::SetKey texture_descriptor_set_key = texture.descriptor_set_key();
  descriptor::SetIndexHint set_index_hint{0};
  if (!preferred_descriptor_sets.empty())
    set_index_hint = get_set_index_hint(preferred_descriptor_sets[0].descriptor_set_key());
  Dout(dc::vulkan, "Using SetIndexHint " << set_index_hint);
  m_shader_resource_set_key_to_set_index_hint.try_emplace_set_index_hint(texture_descriptor_set_key, set_index_hint);

  shader_builder::ShaderResourceDeclaration shader_resource_tmp(texture.glsl_id_full(), vk::DescriptorType::eCombinedImageSampler, set_index_hint, texture);

  auto res1 = m_glsl_id_to_shader_resource.insert(std::pair{texture.glsl_id_full(), shader_resource_tmp});
  // The m_glsl_id_full of each Texture must be unique. And of course, don't register the same texture twice.
  ASSERT(res1.second);

  shader_builder::ShaderResourceDeclaration* shader_resource_ptr = &res1.first->second;
  Dout(dc::vulkan, "Using ShaderResourceDeclaration* " << shader_resource_ptr);
  m_shader_resource_set_key_to_shader_resource_declaration.try_emplace_declaration(texture_descriptor_set_key, shader_resource_ptr);

  // Texture only has a single member.
  shader_resource_ptr->add_member(texture.member());
  // Which is treated here in a general way (but really shader_resource_variables() has just a size of one).
  for (auto& shader_resource_variable : shader_resource_ptr->shader_resource_variables())
    m_shader_variables.push_back(&shader_resource_variable);

  Dout(dc::always, "m_shader_variables (@" << (void*)&m_shader_variables << " [" << this << "]) currently contains:");
  for (auto shader_variable_ptr : m_shader_variables)
    Dout(dc::always, "  " << vk_utils::print_pointer(shader_variable_ptr));

  // Add a ShaderResourceDeclarationContext with key set_index_hint, if that doesn't already exists.
  if (m_set_index_hint_to_shader_resource_declaration_context.find(set_index_hint) == m_set_index_hint_to_shader_resource_declaration_context.end())
  {
    DEBUG_ONLY(auto res2 =)
      m_set_index_hint_to_shader_resource_declaration_context.try_emplace(set_index_hint, this);
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }

  // Remember that this texture must be bound to its descriptor set from the PipelineFactory.
  request_shader_resource_creation(&texture);
}

void ShaderInputData::add_uniform_buffer(shader_builder::shader_resource::UniformBufferBase const& uniform_buffer,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "ShaderInputData::add_uniform_buffer(" << uniform_buffer << ", " << preferred_descriptor_sets << ", " << undesirable_descriptor_sets << ")");
  //FIXME: implement (use preferred_descriptor_sets / undesirable_descriptor_sets).
  descriptor::SetKey uniform_buffer_descriptor_set_key = uniform_buffer.descriptor_set_key();
  descriptor::SetIndexHint set_index_hint = m_shader_resource_set_key_to_set_index_hint.try_emplace_set_index_hint(uniform_buffer_descriptor_set_key);
  Dout(dc::vulkan, "Using SetIndexHint " << set_index_hint);

  shader_builder::ShaderResourceDeclaration shader_resource_tmp(uniform_buffer.glsl_id(), vk::DescriptorType::eUniformBuffer, set_index_hint, uniform_buffer);

  auto res1 = m_glsl_id_to_shader_resource.insert(std::pair{uniform_buffer.glsl_id(), shader_resource_tmp});
  // The m_glsl_id_prefix of each UniformBuffer must be unique. And of course, don't register the same uniform buffer twice.
  ASSERT(res1.second);
  shader_builder::ShaderResourceDeclaration* shader_resource_ptr = &res1.first->second;
  m_shader_resource_set_key_to_shader_resource_declaration.try_emplace_declaration(uniform_buffer_descriptor_set_key, shader_resource_ptr);

  shader_resource_ptr->add_members(uniform_buffer.members());
  for (auto const& shader_resource_variable : shader_resource_ptr->shader_resource_variables())
    m_shader_variables.push_back(&shader_resource_variable);

  //FIXME: remove code duplication (this also exists in add_texture).
  // Add a ShaderResourceDeclarationContext with key set_index_hint, if that doesn't already exists.
  if (m_set_index_hint_to_shader_resource_declaration_context.find(set_index_hint) == m_set_index_hint_to_shader_resource_declaration_context.end())
  {
    DEBUG_ONLY(auto res2 =)
      m_set_index_hint_to_shader_resource_declaration_context.try_emplace(set_index_hint, this);
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }

  // Remember that this uniform buffer must be created from the PipelineFactory.
  request_shader_resource_creation(&uniform_buffer);
}

void ShaderInputData::request_shader_resource_creation(shader_builder::shader_resource::Base const* shader_resource)
{
  // Add a thread-safe pointer to the shader resource (Base) to a list of required shader resources.
  // The shader_resource should be an instance of the Window class.
  m_required_shader_resources_list.push_back(shader_resource);
}

bool ShaderInputData::sort_required_shader_resources_list()
{
  // Seems impossible that one of these is empty if the other isn't.
  ASSERT(m_required_shader_resources_list.empty() == m_sorted_descriptor_set_layouts.empty());

  if (m_required_shader_resources_list.empty())
  {
    // Initialize m_set_index_end because we won't call handle_shader_resource_creation_requests.
    m_set_index_end.set_to_zero();
    return true;        // Nothing to do.
  }

  // The used resources need to have some global order for the "horse race" algorithm to work.
  // Sorting them by Base::create_access_t* will do.
  std::sort(m_required_shader_resources_list.begin(), m_required_shader_resources_list.end());

  // Continue with calling handle_shader_resource_creation_requests.
  return false;
}

// Returns true if this PipelineFactory task succeeded in creating all shader resources needed by the current pipeline (being created),
// and false if one or more other tasks are creating one or more of those shader resources at this moment.
// Upon successful return m_descriptor_set_per_set_index was initialized with handles to the descriptor sets needed by the current pipeline.
// Additionally, those descriptor sets are updated to bind to the respective shader resources. The descriptor sets are created
// as needed: if there already exists a descriptor set that is bound to the same shader resources that we need, we just reuse
// that descriptor set.
bool ShaderInputData::handle_shader_resource_creation_requests(task::PipelineFactory* pipeline_factory, task::SynchronousWindow const* owning_window, vulkan::descriptor::SetBindingMap const& set_binding_map)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "ShaderInputData::handle_shader_resource_creation_requests(" << pipeline_factory << ", " << owning_window << ", " << set_binding_map << ")");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  // Just (re)initialize m_set_index_end every time we (re)enter this function.
  m_set_index_end.set_to_zero();
  m_set_index.set_to_zero();

  // Try to be the first to get 'ownership' on each shader resource that has to be created.
  // This is achieved in the following way:
  // m_required_shader_resources_list is sorted by (unique) pointer value of the shader resources
  // so that every pipeline factory will try to process them in the same order.
  // If, for example, pipeline factory 0 tries to create A, B, C, D; and pipeline factory 1
  // tries to create B, D, E then one that locks B first wins. And if at the same time pipeline
  // factory 2 tries to create C, E, then the winner of B competes with 2 in a race for
  // C or E respectively. I dubbed this the "race horse" algorithm because each shader resource
  // plays the role of a finish line:
  //
  // 0 ---A-B-C-D---->  needs to lock B before 1 and then C before 2.
  //        | | |
  // 1 -----B-|-D-E-->  needs to lock B before 0 and then E before 2.
  //          |   |
  // 2 -------C---E-->  needs to lock C before 0 and E before 1.
  //
  for (Base const* shader_resource : m_required_shader_resources_list)
  {
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = set_binding_map.convert(set_index_hint);

    if (set_index >= m_set_index_end)
      m_set_index_end = set_index + 1;

    if (shader_resource->is_created())  // Skip shader resources that are already created.
      continue;

    // Try to get the lock on all other shader resources.
    if (!shader_resource->acquire_create_lock(pipeline_factory, task::PipelineFactory::obtained_create_lock))
    {
      // If some other task was first, then we back off by returning false.
      return false;
    }
    // Now that we have the exclusive lock - it is safe to const_cast the const-ness away.
    // This is only safe because no other thread will be reading the (non-atomic) members
    // of the shader resources that we are creating while we are creating them.
    m_acquired_required_shader_resources_list.push_back(const_cast<Base*>(shader_resource));
  }

#ifdef CWDEBUG
  int index = 0;
#endif
  for (Base* shader_resource : m_acquired_required_shader_resources_list)
  {
    // Create the shader resource (if not already created (e.g. Texture)).
    shader_resource->instantiate(owning_window COMMA_CWDEBUG_ONLY(owning_window->debug_name_prefix(shader_resource->debug_name())));

    // Let other pipeline factories know that this shader resource was already created.
    // It might be used immediately, even during this call.
    shader_resource->set_created();
    Debug(++index);
  }

  // Release all the task-mutexes, in reverse order.
  for (auto shader_resource = m_acquired_required_shader_resources_list.rbegin(); shader_resource != m_acquired_required_shader_resources_list.rend(); ++shader_resource)
    (*shader_resource)->release_create_lock();

  return true;
}

void ShaderInputData::initialize_shader_resources_per_set_index(vulkan::descriptor::SetBindingMap const& set_binding_map)
{
  DoutEntering(dc::vulkan, "ShaderInputData::initialize_shader_resources_per_set_index(" << set_binding_map << ")");
  Dout(dc::shaderresource, "m_set_index_end = " << m_set_index_end);

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  m_shader_resources_per_set_index.resize(m_set_index_end.get_value());
  Dout(dc::shaderresource, "Running over all m_required_shader_resources_list shader_resource's:");
  for (Base const* shader_resource : m_required_shader_resources_list)
  {
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = set_binding_map.convert(set_index_hint);
    Dout(dc::shaderresource, "  shader_resource " << shader_resource << " (" << print_string(shader_resource->debug_name()) <<
        ") has set_index_hint = " << set_index_hint << ", set_index = " << set_index);
    m_shader_resources_per_set_index[set_index].push_back(shader_resource);
  }
  // Sort the vectors inside m_shader_resources_per_set_index so that pipeline factories that attempt
  // to create the same descriptor set will process the associated shader resources in the same order.
  for (Base const* shader_resource : m_required_shader_resources_list)
  {
    SetKey const set_key = shader_resource->descriptor_set_key();
    SetIndexHint const set_index_hint = get_set_index_hint(set_key);
    SetIndex const set_index = set_binding_map.convert(set_index_hint);
    std::sort(m_shader_resources_per_set_index[set_index].begin(), m_shader_resources_per_set_index[set_index].end());
  }

  // We're going to fill this vector now.
  ASSERT(m_descriptor_set_per_set_index.empty());
  m_descriptor_set_per_set_index.resize(m_set_index_end.get_value());
}

bool ShaderInputData::update_missing_descriptor_sets(task::PipelineFactory* pipeline_factory, task::SynchronousWindow const* owning_window, vulkan::descriptor::SetBindingMap const& set_binding_map, bool have_lock)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "ShaderInputData::update_missing_descriptor_sets(" << pipeline_factory << ", " << owning_window << ", " << set_binding_map << ") [" << pipeline_factory << "]");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  utils::Vector<vk::DescriptorSetLayout, SetIndex> vhv_descriptor_set_layouts = get_vhv_descriptor_set_layouts(set_binding_map);
  // Isn't this ALWAYS the case? Note that it might take a complex application that is skipping
  // shader resources that are in a given descriptor set but aren't used in this pipeline; hence
  // leave this assert here for a very long time.
  ASSERT(m_set_index_end == vhv_descriptor_set_layouts.iend());

  std::vector<vk::DescriptorSetLayout> missing_descriptor_set_layouts;          // The descriptor set layout handles of descriptor sets that we need to create.
  std::vector<std::pair<SetIndex, bool>> set_index_has_frame_resource_pairs;    // The corresponding SetIndex-es, and a flag indicating if this is a frame resource, of those descriptor sets.

  // Descriptor sets: { U1, T1 }, { U2, T2 }, { T1, T2 }, { T2, T1 }, { U1, U2 }, { U2, U1 }, { U1, T2 }, { U2, T1 }
  // Pipeline layout:                   SetLayout
  //   ({ U, T }, { U, T }),            (#0{ U, T }, #1{ U, T })
  //   ({ U, U }, { T, T }),            (#0{ U, U }, #1{ T, T })
  //   ({ T, T }, { U, U })             (#0{ T, T }, #1{ U, U })
  // Pipelines:
  //   ({ U1, T1 }, { U2, T2 }),
  //   ({ U1, T2 }, { U2, T1 }),
  //   ({ U2, T1 }, { U1, T2 }),
  //   ({ U2, T2 }, { U1, T1 }),
  //   ({ U1, U2 }, { T1, T2 }),
  //   ({ U1, U2 }, { T2, T1 }),
  //   ({ U2, U1 }, { T2, T1 }),  <-- 1st      #0 -> #0, #1 -> #1       U2 -> <#0{ U, U }, 0>,
  //                                                                    U1 -> <#0{ U, U }, 1>,
  //                                                                    T2 -> <#1{ T, T }, 0>,
  //                                                                    T1 -> <#1{ T, T }, 1>
  //   ({ U2, U1 }, { T1, T2 }),
  //   ({ T1, T2 }, { U1, U2 }),
  //   ({ T1, T2 }, { U2, U1 }),  <-- 2nd      #0 -> #1, #1 -> #0       T1 -> <#0{ T, T }, 0>,
  //                                                                    T2 -> <#0{ T, T }, 1>,
  //                                                                    U2 -> <#1{ U, U }, 0>,
  //                                                                    U1 -> <#1{ U, U }, 1>
  //   ({ T2, T1 }, { U1, U2 }),
  //   ({ T2, T1 }, { U2, U1 })
  //
  //    U2 -> <#0{ U, U }, 0>,
  //    U1 -> <#0{ U, U }, 1>,
  // T  T2 -> <#1{ T, T }, 0>, <#0{ T, T }, 1>, ...
  // T  T1 -> <#1{ T, T }, 1>, <#0{ T, T }, 0>, ...
  //
  Dout(dc::shaderresource, "Locking and searching for existing descriptor sets: loop over set_index [" << m_set_index << " - " << m_set_index_end << ">");
  for (SetIndex set_index = m_set_index; set_index < m_set_index_end; ++set_index)    // #0, #1, #2, #3
  {
    // We want to know if there is a vk::DescriptorSetLayout that appears in every shader resource corresponding to this set_index.
    std::map<descriptor::FrameResourceCapableDescriptorSetAsKey, int> number_of_shader_resources_bound_to;
    int number_of_shader_resources_in_this_set_index = 0;
    descriptor::SetLayoutBinding locked_set_layout_binding{VK_NULL_HANDLE, 0};
    bool have_match = false;
    descriptor::FrameResourceCapableDescriptorSet existing_descriptor_set;
    // Shader resources: T1, T2, T3, T4, U1, U2, U3, U4 (each with a unique key)
    // PL0:              PL1:                    PL1 after conversion:
    // #0 : { U2, U1 }   #0 : { U4, U3 } -> #0   #0 : { U4, U3 }
    // #1 : { T2, T1 }   #1 : { U2, U1 } -> #2   #1 : { T4, T3 }
    // #2 : { U3, U4 }   #2 : { T1, T2 } -> #3   #2 : { U2, U1 }
    // #3 : { T4, T3 }   #3 : { T4, T3 } -> #1   #3 : { T1, T2 }                  // set index: #0        #1        #2        #3
    Dout(dc::shaderresource, "Loop over all shader resources with index " << set_index);
    bool first_shader_resource = true;
    bool has_frame_resource = false;
    auto shader_resource_iter = m_shader_resources_per_set_index[set_index].begin();
    for (; shader_resource_iter != m_shader_resources_per_set_index[set_index].end(); ++shader_resource_iter)
    {
      Base const* shader_resource = *shader_resource_iter;
      Dout(dc::shaderresource, "shader_resource = " << shader_resource << " (" << print_string(shader_resource->debug_name()) << ")");
      SetKey const set_key = shader_resource->descriptor_set_key();
      SetIndexHint const set_index_hint = get_set_index_hint(set_key);
      uint32_t binding = get_declaration(set_key)->binding();
      Dout(dc::shaderresource, "binding = " << binding);

      if (shader_resource->is_frame_resource())
        has_frame_resource = true;

      // Find the corresponding SetLayout.
      auto sorted_descriptor_set_layout = std::find_if(m_sorted_descriptor_set_layouts.begin(), m_sorted_descriptor_set_layouts.end(), CompareHint{set_index_hint});
      ASSERT(sorted_descriptor_set_layout != m_sorted_descriptor_set_layouts.end());
      // cp0 --> { U, U }, cp1 --> { T, T }
      vk::DescriptorSetLayout descriptor_set_layout = sorted_descriptor_set_layout->handle();
      Dout(dc::shaderresource, "shader_resource corresponds with descriptor_set_layout " << descriptor_set_layout);
      descriptor::SetLayoutBinding const set_layout_binding(descriptor_set_layout, binding);
      Dout(dc::shaderresource, "set_layout_binding (key) = " << set_layout_binding);
      // Assume a given set index requires four shader resources Rx.
      // Since m_shader_resources_per_set_index[set_index] is sorted by descriptor set layout handle, all pipeline
      // factories will process the shader resources in the same order; lets say R1, R2, R3 and finally R4.
      //
      // A set then might look like:
      //             { R3, R1, R2, R4 }
      //       binding: 0   1   2   3
      //                        |
      //                        |
      //                        v
      //                   {T,T,U,U}/2 --> SetMutexAndSetHandles = L{ handle1, handle2 }
      //  SetLayoutBinding:   layout/binding
      //
      // Note that SetLayoutBinding in reality stores a vk::DescriptorSetLayout handle, and not the layout itself, but
      // since we use a cache (LogicalDevice::m_descriptor_set_layouts) for descriptor set layouts, each unique layout
      // will have a unique handle.
      //
      // The function shader_resource->lock_set_layout_binding attempts to add a {SetLayoutBinding, SetMutexAndSetHandles}
      // pair to m_set_layout_bindings_to_handles using the descriptor set layout handle and binding that correspond
      // to the required descriptor set.
      //
      // If the key already exists, and the corresponding SetMutexAndSetHandles is locked, it returns false.
      // If it didn't exist, it was created. If the corresponding SetMutexAndSetHandles wasn't locked, it was
      // locked and returned true; this signifies that creation of this descriptor set might be in
      // progress.
      //
      // Hence the state of the first shader resource, R1, before calling lock_set_layout_binding is either:
      // 1) the key {descriptor_set_layout, binding} does not exist.
      // 2) the key exists with a SetMutexAndSetHandles that isn't locked.
      // 3) the key exists with a SetMutexAndSetHandles that is locked.
      //
      // In case 1) the key is created with an empty SetMutexAndSetHandles, which is locked.
      // In case 2) the corresponding SetMutexAndSetHandles is also locked.
      //
      // The current pipeline factory then should do the following:
      // 1,2) Continue with next shader resource until it we find a case 3) or finished checking all shader resources.
      // 3) Back off - until the lock is released. At which point we should continue as in 2). When the other task
      //    released the lock and this task acquired it, this task will re-enter update_missing_descriptor_sets from
      //    the top with have_lock set to true.

      // If this is not the first shader resource (R1) then it is impossible that other tasks are trying to create
      // the same descriptor set: they process the shader_resources involved in the same order and will stop when
      // trying the first shader resource (R1) or had stopped us if they were first. Therefore no lock needs to
      // obtained for R2, R3 and R4.
      bool in_progress =
        first_shader_resource &&        // Only lock the first shader_resource for every set_index.
        !have_lock &&                   // Already have the lock; no need to call lock_set_layout_binding.
        !shader_resource->lock_set_layout_binding(set_layout_binding, pipeline_factory, task::PipelineFactory::obtained_set_layout_binding_lock);
      if (first_shader_resource)
      {
#ifdef CWDEBUG
        if (have_lock)
          shader_resource->set_have_lock(set_layout_binding, pipeline_factory);
#endif
        first_shader_resource = false;
        Dout(dc::shaderresource, (in_progress ? "In progress!" : "Not in progress."));
        if (in_progress)
        {
          // If the first shader resource (R1) was already locked, and this is not the first
          // set_index that we processed this call (which is m_set_index) then continue with
          // handling the set indexes that we already processed: [m_set_index, set_index>.
          if (set_index > m_set_index)
            allocate_update_add_handles_and_unlocking(pipeline_factory, owning_window, set_binding_map, missing_descriptor_set_layouts, set_index_has_frame_resource_pairs, m_set_index, set_index);
          // Next time continue with the current set_index.
          m_set_index = set_index;
          return false; // Wait until the other pipeline factory unlocked shader_resource->m_set_layout_bindings_to_handles[set_layout_binding].
        }

        // Remember which set_layout_binding we locked for this set_index.
        locked_set_layout_binding = set_layout_binding;
      }

      ++number_of_shader_resources_in_this_set_index;
      bool still_have_match = false;
      {
        auto set_layout_bindings_to_handles_crat = shader_resource->set_layout_bindings_to_handles();
        auto set_layout_binding_handles_pair_iter = set_layout_bindings_to_handles_crat->find(set_layout_binding);
        if (set_layout_binding_handles_pair_iter == set_layout_bindings_to_handles_crat->end())
        {
          Dout(dc::shaderresource, "Setting have_match to false because " << set_layout_binding << " doesn't exist.");
          have_match = false;
          // Note: we need to keep the lock on set_layout_binding in this case (if we have it).
          break;
        }
        else
          Dout(dc::shaderresource, "Found SetMutexAndSetHandles for " << set_layout_binding);

        // Run over all descriptor::FrameResourceCapableDescriptorSet that this shader resource is already bound to.
        auto const& descriptor_sets = set_layout_binding_handles_pair_iter->second.descriptor_sets(CWDEBUG_ONLY(pipeline_factory));
        Dout(dc::shaderresource, "Running over " << descriptor_sets.size() << " descriptor sets.");
        for (descriptor::FrameResourceCapableDescriptorSet const& descriptor_set : descriptor_sets)
        {
          Dout(dc::shaderresource, "Incrementing count for descriptor set " << descriptor_set);
          if (++number_of_shader_resources_bound_to[descriptor_set.as_key()] == number_of_shader_resources_in_this_set_index)
          {
            Dout(dc::shaderresource, "descriptor_set = " << descriptor_set << " and binding = " << binding << " leads to still_have_match = true.");
            have_match = still_have_match = true;
            existing_descriptor_set = descriptor_set;
            Dout(dc::shaderresource, "Set existing_descriptor_set to " << existing_descriptor_set);
          }
          else
          {
            Dout(dc::shaderresource, "number_of_shader_resources_in_this_set_index = " << number_of_shader_resources_in_this_set_index <<
                " but number_of_shader_resources_bound_to[descriptor_set] now is only " << number_of_shader_resources_bound_to[descriptor_set.as_key()]);
          }
        }
      } // Destruct set_layout_bindings_to_handles_crat.

      if (!still_have_match)
      {
        Dout(dc::shaderresource, "Setting have_match to false because still_have_match is false.");
        have_match = false;
        break;
      }
    } // Next shader resource for this set_index.

    if (have_match)
    {
      m_descriptor_set_per_set_index[set_index] = existing_descriptor_set;
      // It should not be possible to get here without locking a set_layout_binding on a shader_resource.
      ASSERT(!m_shader_resources_per_set_index[set_index].empty() && locked_set_layout_binding.descriptor_set_layout());
      (*m_shader_resources_per_set_index[set_index].begin())->unlock_set_layout_binding(locked_set_layout_binding, pipeline_factory);
    }
    else
    {
      missing_descriptor_set_layouts.push_back(vhv_descriptor_set_layouts[set_index]);
      Dout(dc::shaderresource, "Adding set_index " << set_index << " to set_index_has_frame_resource_pairs because have_match is false.");
      // See if there is any shader resource in this set index that is a frame resource.
      for (; !has_frame_resource && shader_resource_iter != m_shader_resources_per_set_index[set_index].end(); ++shader_resource_iter)
        if ((*shader_resource_iter)->is_frame_resource())
          has_frame_resource = true;
      set_index_has_frame_resource_pairs.emplace_back(set_index, has_frame_resource);
    }
    // This is only true when we just (re)entered this function, not for the next set_index.
    have_lock = false;
  }

  allocate_update_add_handles_and_unlocking(pipeline_factory, owning_window, set_binding_map, missing_descriptor_set_layouts, set_index_has_frame_resource_pairs, m_set_index, m_set_index_end);
  return true;
}

void ShaderInputData::allocate_update_add_handles_and_unlocking(task::PipelineFactory* pipeline_factory, task::SynchronousWindow const* owning_window, vulkan::descriptor::SetBindingMap const& set_binding_map, std::vector<vk::DescriptorSetLayout> const& missing_descriptor_set_layouts, std::vector<std::pair<descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs, descriptor::SetIndex set_index_begin, descriptor::SetIndex set_index_end)
{
  DoutEntering(dc::shaderresource|dc::vulkan, "ShaderInputData::allocate_update_add_handles_and_unlocking(" << pipeline_factory << ", " << owning_window << ", " << missing_descriptor_set_layouts << ", " << set_index_has_frame_resource_pairs << ")");

  using namespace vulkan::descriptor;
  using namespace vulkan::shader_builder::shader_resource;

  LogicalDevice const* logical_device = owning_window->logical_device();
  std::vector<FrameResourceCapableDescriptorSet> missing_descriptor_sets;
  if (!missing_descriptor_set_layouts.empty())
    missing_descriptor_sets = logical_device->allocate_descriptor_sets(
        owning_window->max_number_of_frame_resources(),
        missing_descriptor_set_layouts, set_index_has_frame_resource_pairs, logical_device->get_descriptor_pool()
        COMMA_CWDEBUG_ONLY(Ambifix{"ShaderInputData::m_descriptor_set_per_set_index", as_postfix(this)}));
           // Note: the debug name is changed when copying this vector to the Pipeline it will be used with.
  for (int i = 0; i < missing_descriptor_set_layouts.size(); ++i)
    m_descriptor_set_per_set_index[set_index_has_frame_resource_pairs[i].first] = missing_descriptor_sets[i];

  Dout(dc::shaderresource, "Updating, adding handles and unlocking: loop over set_index [" << set_index_begin << " - " << set_index_end << ">.");
  for (SetIndex set_index = set_index_begin; set_index < set_index_end; ++set_index)
  {
    // Run over all shader resources in reverse, so that the first one which is the one that is locked, is processed last!
    Dout(dc::shaderresource, "Loop over all shader resources - in reverse - with index " << set_index);
    for (int i = m_shader_resources_per_set_index[set_index].size() - 1; i >= 0; --i)
    {
      Base const* shader_resource = m_shader_resources_per_set_index[set_index][i];
      bool first_shader_resource = i == 0;
      SetKey const set_key = shader_resource->descriptor_set_key();
      SetIndexHint const set_index_hint = get_set_index_hint(set_key);
      ASSERT(set_index == set_binding_map.convert(set_index_hint));
      uint32_t binding = get_declaration(set_key)->binding();
      Dout(dc::shaderresource, "shader_resource = " << shader_resource << " (" << print_string(shader_resource->debug_name()) <<
          "); set_index = " << set_index << "; binding = " << binding);

      auto new_descriptor_set = std::find_if(set_index_has_frame_resource_pairs.begin(), set_index_has_frame_resource_pairs.end(), [=](std::pair<SetIndex, bool> const& p) { return p.first == set_index; });
      if (new_descriptor_set != set_index_has_frame_resource_pairs.end())
      {
        Dout(dc::shaderresource, "The set_index was found in set_index_has_frame_resource_pairs.");
        // Bind it to a descriptor set.
        shader_resource->update_descriptor_set(owning_window, m_descriptor_set_per_set_index[set_index], binding, new_descriptor_set->second);

        // Find the corresponding SetLayout.
        auto sorted_descriptor_set_layout =
          std::find_if(m_sorted_descriptor_set_layouts.begin(), m_sorted_descriptor_set_layouts.end(), CompareHint{set_index_hint});
        vk::DescriptorSetLayout descriptor_set_layout = sorted_descriptor_set_layout->handle();
        Dout(dc::shaderresource, "descriptor_set_layout = " << descriptor_set_layout);
        // Store the descriptor set handle corresponding to the used vk::DescriptorSetLayout (SetLayout) and binding number.
        // This unlocks the '{ descriptor_set_layout, binding }' key when the last argument is non-NULL.
        shader_resource->add_set_layout_binding({ descriptor_set_layout, binding }, m_descriptor_set_per_set_index[set_index],
            first_shader_resource ? pipeline_factory : nullptr);
      }
      else
        Dout(dc::shaderresource, "The set_index was not found in set_index_has_frame_resource_pairs.");
    }
  }
}

void ShaderInputData::build_shader(task::SynchronousWindow const* owning_window,
    shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler, shader_builder::SPIRVCache& spirv_cache, descriptor::SetBindingMap const& set_binding_map
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  DoutEntering(dc::vulkan, "ShaderInputData::build_shader(" << owning_window << ", " << shader_index << ", ...) [" << this << "]");

  std::string glsl_source_code_buffer;
  std::string_view glsl_source_code;
  shader_builder::ShaderInfo const& shader_info = owning_window->application().get_shader_info(shader_index);
  glsl_source_code = preprocess2(shader_info, glsl_source_code_buffer, set_binding_map);

  // Add a shader module to this pipeline.
  spirv_cache.compile(glsl_source_code, compiler, shader_info);
  m_shader_modules.push_back(spirv_cache.create_module({}, owning_window->logical_device()
      COMMA_CWDEBUG_ONLY(".m_shader_modules[" + std::to_string(m_shader_modules.size()) + "]" + ambifix)));
  m_shader_stage_create_infos.push_back(
    {
      .flags = vk::PipelineShaderStageCreateFlags(0),
      .stage = shader_info.stage(),
      .module = *m_shader_modules.back(),
      .pName = "main"
    }
  );
}

} // namespace vulkan::pipeline
