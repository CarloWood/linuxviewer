#include "sys.h"
#include "ShaderInputData.h"
#include "SynchronousWindow.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "utils/malloc_size.h"
#include "debug.h"
#include <cstring>

namespace vulkan::pipeline {

std::string_view ShaderInputData::preprocess(
    shader_builder::ShaderInfo const& shader_info,
    std::string& glsl_source_code_buffer)
{
  DoutEntering(dc::vulkan, "ShaderInputData::preprocess(" << shader_info << ", glsl_source_code_buffer) [" << this << "]");

  std::string_view const source = shader_info.glsl_template_code();

  // Assume no preprocessing is necessary if the source already starts with "#version".
  if (source.starts_with("#version"))
    return source;

  // Store each position where a match string occurs, together with an std::pair
  // containing the found substring that has to be replaced (first) and the
  // string that it has to be replaced with (second).
  std::map<size_t, std::pair<std::string, std::string>> positions;

  // Store all the declaration contexts that are involved.
  std::set<shader_builder::DeclarationContext const*> declaration_contexts;

  int id_to_name_growth = 0;

  // m_shader_variables contains a number of strings that we need to find in the source.
  // They may occur zero or more times.
  for (shader_builder::ShaderVariable const* shader_variable : m_shader_variables)
  {
    std::string match_string = shader_variable->glsl_id_str();
    int count = 0;
    for (size_t pos = 0; (pos = source.find(match_string, pos)) != std::string_view::npos; pos += match_string.length())
    {
      id_to_name_growth += shader_variable->name().length() - match_string.length();
      positions[pos] = std::make_pair(match_string, shader_variable->name());
      ++count;
    }
    // Skip vertex attribute declarations that were already added above.
    if (count > 0)
      declaration_contexts.insert(&shader_variable->is_used_in(shader_info.stage(), this));
    // VertexAttribute PushConstant
  }
  std::string declarations;
  for (shader_builder::DeclarationContext const* declaration_context : declaration_contexts)
    declarations += declaration_context->generate(shader_info.stage());
  declarations += '\n';

  static constexpr char const* version_header = "#version 450\n\n";
  size_t final_source_code_size = std::strlen(version_header) + declarations.size() + source.length() + id_to_name_growth;

  glsl_source_code_buffer.reserve(utils::malloc_size(final_source_code_size + 1) - 1);
  glsl_source_code_buffer = version_header;
  glsl_source_code_buffer += declarations;

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
  for (auto vertex_attribute_iter = m_glsl_id_str_to_vertex_attribute.begin(); vertex_attribute_iter != m_glsl_id_str_to_vertex_attribute.end(); ++vertex_attribute_iter)
  {
    shader_builder::VertexAttribute const& vertex_attribute = vertex_attribute_iter->second;
    auto iter = m_vertex_shader_location_context.locations().find(&vertex_attribute);
    if (iter == m_vertex_shader_location_context.locations().end())
    {
      Dout(dc::warning|continued_cf, "Could not find " << (void*)&vertex_attribute << " (" << vertex_attribute.glsl_id_str() <<
          ") in m_vertex_shader_location_context.locations(), which contains only {");
      for (auto&& e : m_vertex_shader_location_context.locations())
        Dout(dc::continued, "{" << e.first << " (" << e.first->glsl_id_str() << "), location:" << e.second << "}");
      Dout(dc::finish, "}");
      continue;
    }
    // Should have been added by the call to context.update_location(this) in VertexAttributeDeclarationContext::glsl_id_str_is_used_in()
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
  descriptor::SetIndex set_index = m_shader_resource_set_key_to_set_index.try_emplace(texture_descriptor_set_key);
  Dout(dc::vulkan, "Using SetIndex " << set_index);

  shader_builder::ShaderResource shader_resource_tmp("Texture", vk::DescriptorType::eCombinedImageSampler, set_index);

  auto res1 = m_glsl_id_str_to_shader_resource.insert(std::pair{texture.glsl_id_str(), shader_resource_tmp});
  // The m_glsl_id_str of each Texture must be unique. And of course, don't register the same texture twice.
  ASSERT(res1.second);

  shader_builder::ShaderResource* shader_resource_ptr = &res1.first->second;
  shader_builder::ShaderResourceMember const* shader_resource_member_ptr = shader_resource_ptr->add_member(texture.glsl_id_str().c_str());

  m_shader_variables.push_back(shader_resource_member_ptr);

  // Add a ShaderResourceDeclarationContext with key set_index, if that doesn't already exists.
  if (m_set_index_to_shader_resource_declaration_context.find(set_index) == m_set_index_to_shader_resource_declaration_context.end())
  {
    DEBUG_ONLY(auto res2 =)
      m_set_index_to_shader_resource_declaration_context.try_emplace(set_index, this);
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

void ShaderInputData::add_uniform_buffer(shader_builder::shader_resource::UniformBufferBase const& uniform_buffer,
    std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
    std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets)
{
  DoutEntering(dc::vulkan, "ShaderInputData::add_uniform_buffer(" << uniform_buffer << ", " << preferred_descriptor_sets << ", " << undesirable_descriptor_sets << ")");
  //FIXME: implement.
  descriptor::SetKey uniform_buffer_descriptor_set_key = uniform_buffer.descriptor_set_key();
  descriptor::SetIndex set_index = m_shader_resource_set_key_to_set_index.try_emplace(uniform_buffer_descriptor_set_key);
  Dout(dc::vulkan, "Using SetIndex " << set_index);

  std::vector<char const*> const& glsl_id_strs = uniform_buffer.glsl_id_strs();
  ASSERT(!glsl_id_strs.empty());
  std::string glsl_id_prefix(glsl_id_strs[0], std::strchr(glsl_id_strs[0], ':') - glsl_id_strs[0]);
  shader_builder::ShaderResource shader_resource_tmp(glsl_id_prefix, vk::DescriptorType::eUniformBuffer, set_index);

  auto res1 = m_glsl_id_str_to_shader_resource.insert(std::pair{glsl_id_prefix, shader_resource_tmp});
  // The m_glsl_id_prefix of each UniformBuffer must be unique. And of course, don't register the same uniform buffer twice.
  ASSERT(res1.second);
  shader_builder::ShaderResource* shader_resource_ptr = &res1.first->second;

  for (char const* glsl_id_str : glsl_id_strs)
  {
    shader_builder::ShaderResourceMember const* shader_resource_member = shader_resource_ptr->add_member(glsl_id_str);
    m_shader_variables.push_back(shader_resource_member);
  }

  //FIXME: remove code duplication (this also exists in add_texture).
  // Add a ShaderResourceDeclarationContext with key set_index, if that doesn't already exists.
  if (m_set_index_to_shader_resource_declaration_context.find(set_index) == m_set_index_to_shader_resource_declaration_context.end())
  {
    DEBUG_ONLY(auto res2 =)
      m_set_index_to_shader_resource_declaration_context.try_emplace(set_index, this);
    // We just used find() and it wasn't there?!
    ASSERT(res2.second);
  }
}

void ShaderInputData::build_shader(task::SynchronousWindow const* owning_window,
    shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler, shader_builder::SPIRVCache& spirv_cache
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
{
  std::string glsl_source_code_buffer;
  std::string_view glsl_source_code;
  shader_builder::ShaderInfo const& shader_info = owning_window->application().get_shader_info(shader_index);
  glsl_source_code = preprocess(shader_info, glsl_source_code_buffer);

  // Add a shader module to this pipeline.
  spirv_cache.compile(glsl_source_code, compiler, shader_info);
  m_shader_modules.push_back(spirv_cache.create_module({}, owning_window->logical_device()
      COMMA_CWDEBUG_ONLY(ambifix(".m_shader_modules[" + std::to_string(m_shader_modules.size()) + "]"))));
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
