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
} // namespace descriptor
namespace task {
class CombinedImageSamplerUpdater;
} // namespace task

namespace pipeline {

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

  // Filled by user characteristic object to indicate that these shaders are needed
  // for the next pipeline (of the current fill index).
  std::vector<shader_builder::ShaderIndex> m_shaders_that_need_compiling;       // The shaders that need preprocessing and building,
                                                                                // filled by calls to compile.
  // PipelineFactory::m_set_index_hint_map is generated, during PipelineFactory_characteristics_preprocessed,
  // by passing it to LogicalDevice::realize_pipeline_layout, with either an identity map (when a new layout
  // is created) or a map that maps a found existing layout to the one requested, and subsequently, as this
  // pointer, passed to all characteristics that need a do_compile signal and just had their fill index changed
  // (including first time calls of non-range characteristics). Those characteristics that also need do_preprocess
  // then have the CharacteristicRange_preprocess state run followed by CharacteristicRange_compile that calls
  // build_shaders passing this m_set_index_hint_map pointer to build_shader which passes it to preprocess2
  // which uses it to initialize ShaderResourceDeclarationContext::m_set_index_hint_map so that generated
  // declarations will use the correct descriptor set index.
  descriptor::SetIndexHintMap const* m_set_index_hint_map{};

  // Variables used by builder_shaders.
  int m_shaders_that_need_compiling_index;
  shader_builder::ShaderCompiler m_compiler;

  // Filled by preprocess1, used by preprocess2.
  utils::Array<declaration_contexts_container_t, number_of_shader_stage_indexes, ShaderStageIndex> m_per_stage_declaration_contexts;

  int m_context_changed_generation{0};  // Incremented each call to preprocess1 and stored in a context if that was changed.

 protected:
  // Written to by AddVertexShader and AddFragmentShader.
  //
  // A list of all ShaderVariable's (elements of VertexBuffers::m_glsl_id_full_to_vertex_attribute,
  // AddPushConstant::m_glsl_id_full_to_push_constant, PipelineFactory::m_glsl_id_to_shader_resource, ...).
  std::vector<shader_builder::ShaderVariable const*> m_shader_variables;

  // Only access this when your application is not using a PipelineFactory; otherwise it will be used automatically.
  // This is the main output to FlatCreateInfo, filled by build_shader, and contains the shader module handles
  // and the vk::ShaderStageFlagBits that specifies in which stage the module is used.
  std::vector<vk::PipelineShaderStageCreateInfo> m_shader_stage_create_infos;

 private:
  using set_index_hint_to_shader_resource_declaration_context_container_t = std::map<descriptor::SetIndexHint, shader_builder::ShaderResourceDeclarationContext>;
  // Filled by realize_shader_resource_declaration_context for new set_index_hint's.
  set_index_hint_to_shader_resource_declaration_context_container_t m_set_index_hint_to_shader_resource_declaration_context;

 public:
  // Used by ShaderResourceVariable::is_used_in to look up the declaration context by set index hint.
  set_index_hint_to_shader_resource_declaration_context_container_t& set_index_hint_to_shader_resource_declaration_context(utils::Badge<shader_builder::ShaderResourceVariable>) { return m_set_index_hint_to_shader_resource_declaration_context; }

  // Overridden by AddVertexShader.
  virtual shader_builder::VertexAttributeDeclarationContext* vertex_shader_location_context(utils::Badge<shader_builder::VertexAttribute>)
  {
    return nullptr;
  }

  void realize_shader_resource_declaration_context(descriptor::SetIndexHint set_index_hint);

  void prepare_combined_image_sampler_declaration(task::CombinedImageSamplerUpdater const& combined_image_sampler, descriptor::SetIndexHint set_index_hint);
  void prepare_uniform_buffer_declaration(shader_builder::UniformBufferBase const& uniform_buffer, descriptor::SetIndexHint set_index_hint);

  // Accessor.
  int context_changed_generation() const { return m_context_changed_generation; }
  std::vector<vk::PipelineShaderStageCreateInfo> const& shader_stage_create_infos(utils::Badge<ImGui>) const { return m_shader_stage_create_infos; }

 private:
  void set_set_index_hint_map(descriptor::SetIndexHintMap const* set_index_hint_map) override
  {
    DoutEntering(dc::setindexhint, "AddShaderStage::set_set_index_hint_map(" << vk_utils::print_pointer(set_index_hint_map) << ")");
    m_set_index_hint_map = set_index_hint_map;
  }

  // Called from CharacteristicRange_fill_or_terminate.
  void pre_fill_state() override;

  // Called from CharacteristicRange_preprocess.
  vk::ShaderStageFlags preprocess_shaders_and_realize_descriptor_set_layouts(task::PipelineFactory* pipeline_factory) override;

  // Called from CharacteristicRange_compile.
  void start_build_shaders() override { m_compiler.initialize(); m_shaders_that_need_compiling_index = 0; }
  bool build_shaders(task::CharacteristicRange* characteristic_range,
      task::PipelineFactory* pipeline_factory, AIStatefulTask::condition_type locked) override;

  // Called from the top of the first call to preprocess1.
  void prepare_shader_resource_declarations();

  // Override of AddShaderStageBridge.
  void add_shader_variable(shader_builder::ShaderVariable const* shader_variable) override
  {
    m_shader_variables.push_back(shader_variable);
  }

  // This is a AddShaderStage.
  AddShaderStage* get_add_shader_stage() final { return this; }

  // Override of CharacteristicRangeBridge.
  void register_AddShaderStage_with(task::PipelineFactory* pipeline_factory, task::CharacteristicRange const& characteristic_range) const final
  {
    pipeline_factory->add_to_flat_create_info(&m_shader_stage_create_infos, characteristic_range);
  }

  void cache_descriptor_set_layout_bindings(shader_builder::ShaderInfoCache& shader_info_cache);
  void restore_descriptor_set_layout_bindings(shader_builder::ShaderInfoCache const& shader_info_cache, task::PipelineFactory* pipeline_factory);

 protected:
  // Called from *UserCode*PipelineCharacteristic_fill.
  void add_shader(shader_builder::ShaderIndex shader_index);
  void replace_shader(shader_builder::ShaderIndex remove_shader_index, shader_builder::ShaderIndex add_shader_index);

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
      shader_builder::ShaderIndex shader_index,
      shader_builder::ShaderInfoCache& shader_info_cache,
      shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache, descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex shader_index,
      shader_builder::ShaderInfoCache& shader_info_cache,
      shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(Ambifix const& ambifix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, shader_info_cache, compiler, tmp_spirv_cache, set_index_hint_map COMMA_CWDEBUG_ONLY(ambifix));
  }

#ifdef CWDEBUG
  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex shader_index,
      shader_builder::ShaderInfoCache& shader_info_cache,
      shader_builder::ShaderCompiler const& compiler,
      shader_builder::SPIRVCache& spirv_cache,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(std::string prefix))
  {
    build_shader(owning_window, shader_index, shader_info_cache, compiler, spirv_cache, set_index_hint_map
        COMMA_CWDEBUG_ONLY(AmbifixOwner{owning_window, std::move(prefix)}));
  }

  void build_shader(task::SynchronousWindow const* owning_window,
      shader_builder::ShaderIndex shader_index,
      shader_builder::ShaderInfoCache& shader_info_cache,
      shader_builder::ShaderCompiler const& compiler,
      descriptor::SetIndexHintMap const* set_index_hint_map
      COMMA_CWDEBUG_ONLY(std::string prefix))
  {
    shader_builder::SPIRVCache tmp_spirv_cache;
    build_shader(owning_window, shader_index, shader_info_cache, compiler, tmp_spirv_cache, set_index_hint_map
        COMMA_CWDEBUG_ONLY(AmbifixOwner{owning_window, std::move(prefix)}));
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

} // namespace pipeline
} // namespace vulkan
