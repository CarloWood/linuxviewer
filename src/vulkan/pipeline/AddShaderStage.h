#pragma once

#include "CharacteristicRangeBridge.h"
#include "AddShaderStageBridge.h"
#include "shader_builder/ShaderVariable.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/SPIRVCache.h"
#include "shader_builder/ShaderResourceDeclarationContext.h"
#include "descriptor/SetIndex.h"
#include "utils/Badge.h"
#include <map>
#include <vector>
#include <set>
#include "debug.h"

namespace vulkan {
class ImGui;
class AmbifixOwner;
namespace shader_builder {
class ShaderInfo;
class ShaderResourceVariable;
class VertexAttribute;
class VertexAttributeDeclarationContext;
class UniformBufferBase;
} // namespace shader_builder
namespace descriptor {
class SetIndexHintMap;
class CombinedImageSamplerUpdater;
} // namespace descriptor
} // namespace vulkan

namespace vulkan::pipeline {
class CharacteristicRange;

class AddShaderStage : public virtual CharacteristicRangeBridge, public virtual AddShaderStageBridge
{
 private:
  using declaration_contexts_container_t = std::set<shader_builder::DeclarationContext*>;
  declaration_contexts_container_t m_declaration_contexts;

  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;

  vk::UniqueShaderModule m_shader_module;
  vk::PipelineShaderStageCreateInfo m_shader_stage_create_info;

 protected:
  // Written to by AddVertexShader and AddFragmentShader.
  //
  // A list of all ShaderVariable's (elements of m_glsl_id_full_to_vertex_attribute, m_glsl_id_full_to_push_constant,
  // m_glsl_id_to_shader_resource, ...).
  std::vector<shader_builder::ShaderVariable const*> m_shader_variables;

 public:
  using set_index_hint_to_shader_resource_declaration_context_container_t = std::map<descriptor::SetIndexHint, shader_builder::ShaderResourceDeclarationContext>;
  set_index_hint_to_shader_resource_declaration_context_container_t m_set_index_hint_to_shader_resource_declaration_context;
  // Used by ShaderResourceVariable::is_used_in to look up the declaration context.
  set_index_hint_to_shader_resource_declaration_context_container_t& set_index_hint_to_shader_resource_declaration_context(utils::Badge<shader_builder::ShaderResourceVariable>) { return m_set_index_hint_to_shader_resource_declaration_context; }

  // Overridden by AddVertexShader.
  virtual shader_builder::VertexAttributeDeclarationContext* vertex_shader_location_context(utils::Badge<shader_builder::VertexAttribute>)
  {
    return nullptr;
  }

  void realize_shader_resource_declaration_context(descriptor::SetIndexHint set_index_hint);

  void prepare_combined_image_sampler_declaration(descriptor::CombinedImageSamplerUpdater const& combined_image_sampler, descriptor::SetIndexHint set_index_hint);
  void prepare_uniform_buffer_declaration(shader_builder::UniformBufferBase const& uniform_buffer, descriptor::SetIndexHint set_index_hint);

 private:
  // Called from the top of the first call to preprocess1.
  void prepare_shader_resource_declarations();

  // Override of AddShaderStageBridge.
  void add_shader_variable(shader_builder::ShaderVariable const* shader_variable) override
  {
    m_shader_variables.push_back(shader_variable);
  }

 protected:
  // Called from *UserCode*PipelineCharacteristic_initialize.
  void preprocess1(shader_builder::ShaderInfo const& shader_info);

  // Create glsl code from template source code.
  //
  // glsl_source_code_buffer is only used when the code from shader_info needs preprocessing,
  // otherwise this function returns a string_view directly into the shader_info's source code.
  //
  // Hence, both shader_info and the string passed as glsl_source_code_buffer need to have a life time beyond the call to compile.
  std::string_view preprocess2(shader_builder::ShaderInfo const& shader_info, std::string& glsl_source_code_buffer, descriptor::SetIndexHintMap const* set_index_hint_map) const;

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& ambifix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, compiler, tmp_spirv_cache, set_index_hint_map COMMA_CWDEBUG_ONLY(ambifix));
  }
};

} // namespace vulkan::pipeline
