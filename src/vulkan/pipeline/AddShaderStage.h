#pragma once

#include "CharacteristicRangeBridge.h"
#include "AddShaderStageBridge.h"
#include "PipelineFactory.h"
#include "shader_builder/ShaderVariable.h"
#include "shader_builder/ShaderIndex.h"
#include "shader_builder/SPIRVCache.h"
#include "shader_builder/ShaderResourceDeclarationContext.h"
#include "descriptor/SetIndex.h"
#include "utils/Badge.h"
#include "utils/Array.h"
#include "utils/is_power_of_two.h"
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
  using ShaderStageIndex = utils::ArrayIndex<declaration_contexts_container_t>;

  static constexpr ShaderStageIndex eVertex_index{0};
  static constexpr ShaderStageIndex eFragment_index{1};
  static constexpr size_t number_of_shader_stage_indexes{2};
  static constexpr vk::ShaderStageFlagBits largest_shader_stage_flag = vk::ShaderStageFlagBits::eFragment;

  // Convert a vk::ShaderStageFlagBits single-bit mask to the index of s_log2ShaderStageFlagBits_to_ShaderStageIndex.
  static constexpr int get_lookup_index(vk::ShaderStageFlagBits shader_stage_flag_bit) { return utils::log2(static_cast<VkShaderStageFlags>(shader_stage_flag_bit)); }

  static constexpr ShaderStageIndex undefined_shader_stage_index{};
  // Lookup table that converts the result of get_lookup_index to a ShaderStageIndex.
  // Using log2 etc here because we're not allowed to use the "incomplete" get_lookup_index yet :(
  static constexpr std::array<ShaderStageIndex, utils::log2(static_cast<VkShaderStageFlags>(largest_shader_stage_flag)) + 1> const s_log2ShaderStageFlagBits_to_ShaderStageIndex = {
    eVertex_index,
    undefined_shader_stage_index,
    undefined_shader_stage_index,
    undefined_shader_stage_index,
    eFragment_index
  };

  static constexpr ShaderStageIndex ShaderStageFlag_to_ShaderStageIndex(vk::ShaderStageFlagBits shader_stage_flag)
  {
    ASSERT(utils::is_power_of_two(static_cast<VkShaderStageFlags>(shader_stage_flag)));
    return s_log2ShaderStageFlagBits_to_ShaderStageIndex[get_lookup_index(shader_stage_flag)];
  }

#ifdef CWDEBUG
  friend struct StaticCheckLookUpTable;
#endif

  utils::Array<declaration_contexts_container_t, number_of_shader_stage_indexes, ShaderStageIndex> m_per_stage_declaration_contexts;
  utils::Array<vk::UniqueShaderModule, number_of_shader_stage_indexes, ShaderStageIndex> m_per_stage_shader_module;
  int m_context_changed_generation{0};  // Incremented each call to preprocess1 and stored in a context if that was changed.

 protected:
  // Written to by AddVertexShader and AddFragmentShader.
  //
  // A list of all ShaderVariable's (elements of VertexBuffers::m_glsl_id_full_to_vertex_attribute,
  // AddPushConstant::m_glsl_id_full_to_push_constant, PipelineFactory::m_glsl_id_to_shader_resource, ...).
  std::vector<shader_builder::ShaderVariable const*> m_shader_variables;

  // Only access this when your application is not using a PipelineFactory; otherwise it will be used automatically.
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;

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

  // Accessor.
  int context_changed_generation() const { return m_context_changed_generation; }
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos(utils::Badge<ImGui>) const { return m_shader_stage_create_infos; }

 private:
  // Called from the top of the first call to preprocess1.
  void prepare_shader_resource_declarations();

  // Override of AddShaderStageBridge.
  void add_shader_variable(shader_builder::ShaderVariable const* shader_variable) override
  {
    m_shader_variables.push_back(shader_variable);
  }

  // This is a AddShaderStage.
  bool is_add_shader_stage() const final { return true; }

  // Override of CharacteristicRangeBridge.
  void register_AddShaderStage_with(task::PipelineFactory* pipeline_factory) const final
  {
    pipeline_factory->add_to_flat_create_info(&m_shader_stage_create_infos);
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
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, compiler, tmp_spirv_cache, set_index_hint_map COMMA_CWDEBUG_ONLY(ambifix));
  }

#ifdef CWDEBUG
  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(std::string prefix))
  {
    build_shader(owning_window, shader_index, compiler, spirv_cache, set_index_hint_map
        COMMA_CWDEBUG_ONLY(AmbifixOwner{owning_window, std::move(prefix)}));
  }

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex const& shader_index, shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(std::string prefix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, compiler, tmp_spirv_cache, set_index_hint_map
        COMMA_CWDEBUG_ONLY(AmbifixOwner{owning_window, std::move(prefix)}));
  }
#endif

#if 0
  // Not used at the moment: we destruct them when AddShaderStage::m_per_stage_shader_module is destructed, which are vk::UniqueShaderModule.
  void destroy_shader_module_handles() override
  {
    DoutEntering(dc::always, "destroy_shader_module_handles() [" << this << "]");
    for (ShaderStageIndex i = m_per_stage_shader_module.ibegin(); i != m_per_stage_shader_module.iend(); ++i)
      m_per_stage_shader_module[i].reset();
  }
#endif
};

#ifdef CWDEBUG
struct StaticCheckLookUpTable : AddShaderStage
{
  // The above initialization must be in the same order as the vk::ShaderStageFlagBits enum (assuming those
  // are defined with values of 1 << n, where n runs from 0 and up with increments of 1.
  static_assert(ShaderStageFlag_to_ShaderStageIndex(vk::ShaderStageFlagBits::eVertex) == eVertex_index);
  static_assert(ShaderStageFlag_to_ShaderStageIndex(vk::ShaderStageFlagBits::eFragment) == eFragment_index);
};
#endif

} // namespace vulkan::pipeline
