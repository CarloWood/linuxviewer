#pragma once

#include "shaderbuilder/VertexShaderInputSet.h"
#include "shaderbuilder/VertexAttribute.h"
#include "shaderbuilder/ShaderInfo.h"
#include "shaderbuilder/SPIRVCache.h"
#include "debug/DebugSetName.h"
#include "utils/Vector.h"
#include <vector>
#include <set>

namespace task {
class SynchronousWindow;
} // namespace task

namespace vulkan::pipeline {

class Pipeline
{
  utils::Vector<shaderbuilder::VertexShaderInputSetBase*> m_vertex_shader_input_sets;   // Existing vertex shader input sets (a 'binding' slot).
  std::set<shaderbuilder::VertexAttributeEntry> m_vertex_attributes;                    // All existing attributes of the above input sets.
  shaderbuilder::LocationContext m_vertex_shader_location_context;                      // Location context used for m_vertex_attributes.

  // FIXME: these don't belong in this object, or do they?
  task::SynchronousWindow const* m_owning_window = {};
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;
  std::vector<vk::UniqueShaderModule> m_unique_handles;
  // FIXME: these two don't belong in this object.
  std::vector<BufferParameters> m_buffers;
  std::vector<vk::Buffer> m_vhv_buffers;

 public:
  void owning_window(task::SynchronousWindow const* owning_window)
  {
    // Only call set_window once.
    ASSERT(!m_owning_window);
    m_owning_window = owning_window;
  }

  template<typename ENTRY>
  void add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set);

  void build_shader(shaderbuilder::ShaderInfo const& shader_info, shaderbuilder::ShaderCompiler const& compiler, shaderbuilder::SPIRVCache& spirv_cache
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void build_shader(shaderbuilder::ShaderInfo const& shader_info, shaderbuilder::ShaderCompiler const& compiler
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
  {
    shaderbuilder::SPIRVCache tmp_spirv_cache;
    build_shader(shader_info, compiler, tmp_spirv_cache COMMA_CWDEBUG_ONLY(ambifix));
  }

  // Create glsl code from template source code.
  //
  // glsl_source_code_buffer is only used when the code from shader_info needs preprocessing,
  // otherwise this function returns a string_view directly into the shader_info's source code.
  //
  // Hence, both shader_info and the string passed as glsl_source_code_buffer need to have a life time beyond the call to compile.
  std::string_view preprocess(shaderbuilder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer,
      std::set<shaderbuilder::VertexAttributeEntry> const* vertex_attributes = nullptr);

  void generate(task::SynchronousWindow* owning_window
       COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  std::vector<vk::Buffer> const& vhv_buffers() const
  {
    return m_vhv_buffers;
  }

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputBindingDescription> vertex_binding_descriptions() const;

  // Returns information on what was added with add_vertex_input_binding.
  std::vector<vk::VertexInputAttributeDescription> vertex_attribute_descriptions() const;

  // Returns information on what was added with build_shader.
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos() const { return m_shader_stage_create_infos; }
};

template<typename ENTRY>
void Pipeline::add_vertex_input_binding(shaderbuilder::VertexShaderInputSet<ENTRY>& vertex_shader_input_set)
{
  shaderbuilder::BindingIndex binding = m_vertex_shader_input_sets.iend();
  // Register ENTRY as vertex shader input.
  for (auto&& attribute : shaderbuilder::VertexAttributes<ENTRY>::attributes)
  {
    auto res = m_vertex_attributes.emplace(binding, attribute);
    // All used names must be unique.
    if (!res.second)
      THROW_ALERT("Duplicated vertex attribute id \"[ID_STR]\". All used ids must be unique.", AIArgs("[ID_STR]", attribute.m_glsl_id_str));
  }
  // Keep track of all VertexShaderInputSetBase objects.
  m_vertex_shader_input_sets.push_back(&vertex_shader_input_set);
}

} // namespace vulkan::pipeline
