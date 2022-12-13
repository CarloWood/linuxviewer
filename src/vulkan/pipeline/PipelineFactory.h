#ifndef PIPELINE_PIPELINE_FACTORY_H
#define PIPELINE_PIPELINE_FACTORY_H

#include "FlatCreateInfo.h"
#include "Pipeline.h"
#include "ShaderResourcePlusCharacteristic.h"
#include "ShaderResourcePlusCharacteristicIndex.h"
#include "descriptor/SetKeyPreference.h"
#include "descriptor/SetKeyToShaderResourceDeclaration.h"
#include "shader_builder/shader_resource/UniformBuffer.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/RunningTasksTracker.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/MultiLoop.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {
class LogicalDevice;

namespace pipeline {
class FactoryCharacteristicId;
class AddShaderStage;
class CharacteristicRange;
using CharacteristicRangeIndex = utils::VectorIndex<CharacteristicRange>;
} // namespace vulkan::pipeline

namespace shader_builder::shader_resource {
class CombinedImageSampler;
} // namespace shader_builder::shader_resource

namespace descriptor {
class CombinedImageSamplerUpdater;
} // namespace descriptor

} // namespace vulkan

namespace task {
class PipelineCache;

namespace synchronous {
class MoveNewPipelines;
} // namespace synchronous

class PipelineFactory : public AIStatefulTask
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<PipelineFactory>>;
  using characteristics_container_t = utils::Vector<boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange>, vulkan::pipeline::CharacteristicRangeIndex>;
  // The same as CharacteristicRange::pipeline_index_t.
  using pipeline_index_t = aithreadsafe::Wrapper<vulkan::pipeline::Index, aithreadsafe::policy::Primitive<std::mutex>>;

  static constexpr condition_type pipeline_cache_set_up = 1;
  static constexpr condition_type fully_initialized = 2;
  static constexpr condition_type characteristics_initialized = 4;
  static constexpr condition_type characteristics_filled = 8;
  static constexpr condition_type characteristics_preprocessed = 16;
  static constexpr condition_type characteristics_compiled = 32;
  static constexpr condition_type obtained_create_lock = 64;
  static constexpr condition_type obtained_set_layout_binding_lock = 128;

 private:
  // Constructor.
  SynchronousWindow* m_owning_window;
  vk::RenderPass m_vh_render_pass;
  // add.
  characteristics_container_t m_characteristics;
  // Index into SynchronousWindow::m_pipeline_factories, pointing to ourselves.
  PipelineFactoryIndex m_pipeline_factory_index;
  vulkan::pipeline::AddShaderStage* m_add_shader_stage{};       // Virtual base class of the added characteristics.

  // run
  // initialize_impl.
  statefultask::RunningTasksTracker::index_type m_index;
  // State PipelineFactory_start.
  boost::intrusive_ptr<PipelineCache> m_pipeline_cache_task;
  // State PipelineFactory_initialize.
  boost::intrusive_ptr<synchronous::MoveNewPipelines> m_move_new_pipelines_synchronously;
  // State PipelineFactory_initialized.
  vulkan::pipeline::FlatCreateInfo m_flat_create_info;
  utils::Vector<unsigned int, vulkan::pipeline::CharacteristicRangeIndex> m_range_shift;
  MultiLoop m_range_counters;
  int m_start_of_next_loop;
  std::atomic<size_t> m_number_of_running_characteristic_tasks;
  // State PipelineFactory_top_multiloop_while_loop
  vulkan::descriptor::SetIndexHintMap m_set_index_hint_map;
  // State MoveNewPipelines_need_action (which calls set_pipeline).
  vulkan::Pipeline& m_pipeline_out;
  // Index into SynchronousWindow::m_pipelines, enumerating the current pipeline being generated inside the MultiLoop.
  pipeline_index_t m_pipeline_index;
  // Layout of the current pipeline that is being created inside the MultiLoop.
  vk::PipelineLayout m_vh_pipeline_layout;
  // Set to true when calling update_missing_descriptor_sets while already having the set_layout_binding lock for the current pipeline/set_index/first_shader_resource.
  bool m_have_lock;

  struct ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences
  {
    // List of shader resource / Characteristic pairs (range plus range value), where the latter added the former from the add_* functions like add_uniform_buffer.
    vulkan::pipeline::ShaderResourcePlusCharacteristic m_shader_resource_plus_characteristic;
    // Corresponding preferred and undesirable descriptor sets.
    std::vector<vulkan::descriptor::SetKeyPreference> m_preferred_descriptor_sets;
    std::vector<vulkan::descriptor::SetKeyPreference> m_undesirable_descriptor_sets;
  };
  using required_shader_resource_plus_characteristic_list_container_t = utils::Vector<ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences, vulkan::pipeline::ShaderResourcePlusCharacteristicIndex>;
  using required_shader_resource_plus_characteristic_list_t = aithreadsafe::Wrapper<required_shader_resource_plus_characteristic_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  required_shader_resource_plus_characteristic_list_t m_required_shader_resource_plus_characteristic_list;

  //---------------------------------------------------------------------------
  // Shader resources.

  // Set index hints.
  using set_key_to_set_index_hint_container_t = std::map<vulkan::descriptor::SetKey, vulkan::descriptor::SetIndexHint>;
  set_key_to_set_index_hint_container_t m_set_key_to_set_index_hint;                            // Maps descriptor::SetKey's to descriptor::SetIndexHint's.
  using glsl_id_to_shader_resource_container_t = std::map<std::string, vulkan::shader_builder::ShaderResourceDeclaration, std::less<>>;
  using glsl_id_to_shader_resource_t = aithreadsafe::Wrapper<glsl_id_to_shader_resource_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  glsl_id_to_shader_resource_t m_glsl_id_to_shader_resource;  // Set index hint needs to be updated afterwards.
  vulkan::descriptor::SetKeyToShaderResourceDeclaration m_shader_resource_set_key_to_shader_resource_declaration;
     // Maps descriptor::SetKey's to shader resource declaration object used for the current pipeline.
     // Note: write access to this member is hijacking the write-lock on m_glsl_id_to_shader_resource for concurrent access protection.
     // It is read in update_missing_descriptor_sets and allocate_update_add_handles_and_unlocking which doesn't require any locking
     // because nobody is writing to it anymore by then.

  using sorted_descriptor_set_layouts_container_t = std::vector<vulkan::descriptor::SetLayout>;
  using sorted_descriptor_set_layouts_t = aithreadsafe::Wrapper<sorted_descriptor_set_layouts_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  sorted_descriptor_set_layouts_t m_sorted_descriptor_set_layouts;      // A Vector of SetLayout object, containing vk::DescriptorSetLayout
                                                                        // handles and the vk::DescriptorSetLayoutBinding objects, stored in
                                                                        // a sorted vector, that they were created from.

  // Initialized in handle_shader_resource_creation_requests:
  // The list of shader resources that we managed to get the lock on and weren't already created before.
  std::vector<vulkan::shader_builder::ShaderResourceBase*> m_acquired_required_shader_resources_list;
  // Initialized in initialize_shader_resources_per_set_index.
  utils::Vector<std::vector<vulkan::pipeline::ShaderResourcePlusCharacteristic>, vulkan::descriptor::SetIndex> m_shader_resource_plus_characteristics_per_set_index;
  // The largest set_index used in m_required_shader_resource_plus_characteristic_list, plus one.
  vulkan::descriptor::SetIndex m_set_index_end;
  // The current descriptor set index that we're processing in update_missing_descriptor_sets.
  vulkan::descriptor::SetIndex m_set_index;

  // Descriptor set handles, sorted by descriptor set index, required for the next pipeline.
  vulkan::Pipeline::descriptor_set_per_set_index_t m_descriptor_set_per_set_index;

  //---------------------------------------------------------------------------

 private:
  // Called from prepare_shader_resource_declarations.
  void fill_set_index_hints(required_shader_resource_plus_characteristic_list_t::rat const& required_shader_resource_plus_characteristic_list_r, utils::Vector<vulkan::descriptor::SetIndexHint, vulkan::pipeline::ShaderResourcePlusCharacteristicIndex>& set_index_hints_out);
  void prepare_shader_resource_declarations();

  // Returns the SetIndexHint that was assigned to this key (usually by shader_resource::add_*).
  vulkan::descriptor::SetIndexHint get_set_index_hint(vulkan::descriptor::SetKey set_key) const
  {
    auto set_index_hint = m_set_key_to_set_index_hint.find(set_key);
    // Don't call get_set_index_hint for a key that wasn't added yet.
    ASSERT(set_index_hint != m_set_key_to_set_index_hint.end());
    return set_index_hint->second;
  }

  vulkan::shader_builder::ShaderResourceDeclaration const* get_declaration(vulkan::descriptor::SetKey set_key) const
  {
    return m_shader_resource_set_key_to_shader_resource_declaration.get_shader_resource_declaration(set_key);
  }

 public:
  //---------------------------------------------------------------------------
  // Shader resources.

  void add_combined_image_sampler(utils::Badge<vulkan::pipeline::CharacteristicRange>,
      vulkan::shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
      vulkan::pipeline::CharacteristicRange const* adding_characteristic_range,
      std::vector<vulkan::descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<vulkan::descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  void add_uniform_buffer(utils::Badge<vulkan::pipeline::CharacteristicRange>,
      vulkan::shader_builder::UniformBufferBase const& uniform_buffer,
      vulkan::pipeline::CharacteristicRange const* adding_characteristic_range,
      std::vector<vulkan::descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<vulkan::descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  vulkan::shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(utils::Badge<vulkan::pipeline::CharacteristicRange>, std::string glsl_id_full, vk::DescriptorType descriptor_type, vulkan::shader_builder::ShaderResourceBase const& shader_resource, vulkan::descriptor::SetIndexHint set_index_hint);

  void realize_descriptor_set_layouts(utils::Badge<vulkan::pipeline::CharacteristicRange>, vulkan::LogicalDevice const* logical_device);

  void push_back_descriptor_set_layout_binding(vulkan::descriptor::SetIndexHint set_index_hint,
      vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding,
      vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size,
      utils::Badge<vulkan::shader_builder::ShaderResourceDeclarationContext>);

 private:
  // Called by add_combined_image_sampler and/or add_uniform_buffer (at the end), requesting to be created
  // and storing the preferred and undesirable descriptor set vectors.
  void register_shader_resource(vulkan::shader_builder::ShaderResourceBase const* shader_resource,
      vulkan::pipeline::CharacteristicRange const* adding_characteristic_range,
      std::vector<vulkan::descriptor::SetKeyPreference> const& preferred_descriptor_sets,
      std::vector<vulkan::descriptor::SetKeyPreference> const& undesirable_descriptor_sets);

  // End shader resources.
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // Begin of MultiLoop states.

  // Called by PipelineFactory_top_multiloop_while_loop.
  bool sort_required_shader_resources_list();
  // Called by PipelineFactory_create_shader_resources.
  bool handle_shader_resource_creation_requests();
  // Called by PipelineFactory_initialize_shader_resources_per_set_index.
  void initialize_shader_resources_per_set_index();
  // Called by PipelineFactory_update_missing_descriptor_sets.
  bool update_missing_descriptor_sets();
  // Called by update_missing_descriptor_sets.
  void allocate_update_add_handles_and_unlocking(
      std::vector<vk::DescriptorSetLayout> const& missing_descriptor_set_layouts,
      std::vector<uint32_t> const& missing_descriptor_set_unbounded_descriptor_array_size,
      std::vector<std::pair<vulkan::descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs,
      vulkan::descriptor::SetIndex set_index_begin, vulkan::descriptor::SetIndex set_index_end);

  // End of MultiLoop states.
  //---------------------------------------------------------------------------

 protected:
  using direct_base_type = AIStatefulTask;      // The immediate base class of this task.

  // The different states of the task.
  enum PipelineFactory_state_type {
    PipelineFactory_start = direct_base_type::state_end,
    PipelineFactory_initialize,
    PipelineFactory_initialized,
    PipelineFactory_characteristics_initialized,
    PipelineFactory_top_multiloop_for_loop,
    PipelineFactory_top_multiloop_while_loop,
    PipelineFactory_characteristics_filled,
    PipelineFactory_characteristics_preprocessed,
    PipelineFactory_characteristics_compiled,
    PipelineFactory_create_shader_resources,
    PipelineFactory_initialize_shader_resources_per_set_index,
    PipelineFactory_update_missing_descriptor_sets,
    PipelineFactory_bottom_multiloop_while_loop,
    PipelineFactory_bottom_multiloop_for_loop,
    PipelineFactory_done
  };

 public:
  static state_type constexpr state_end = PipelineFactory_done + 1;

 protected:
  ~PipelineFactory() override;

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void multiplex_impl(state_type run_state) override;
  void finish_impl() override;

 public:
  PipelineFactory(SynchronousWindow* owning_window, vulkan::Pipeline& pipeline_out, vk::RenderPass vh_render_pass
      COMMA_CWDEBUG_ONLY(bool debug = false));

  // Accessor.
  SynchronousWindow* owning_window() const { return m_owning_window; }

  vulkan::pipeline::FactoryCharacteristicId add_characteristic(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
  void set_index(PipelineFactoryIndex pipeline_factory_index) { m_pipeline_factory_index = pipeline_factory_index; }
  void set_pipeline(vulkan::Pipeline&& pipeline) { m_pipeline_out = std::move(pipeline); }
  characteristics_container_t const& characteristics() const { return m_characteristics; }
  PipelineFactoryIndex pipeline_factory_index() const { return m_pipeline_factory_index; }

  // Called from CharacteristicRange::multiplex_impl.
  void characteristic_range_initialized();
  void characteristic_range_filled(vulkan::pipeline::CharacteristicRangeIndex index);
  void characteristic_range_preprocessed();
  void characteristic_range_compiled();

  // Called by SynchronousWindow::pipeline_factory_done to rescue the cache, immediately before deleting this task.
  inline boost::intrusive_ptr<PipelineCache> detach_pipeline_cache_task();
};

} // namespace task
#endif // PIPELINE_PIPELINE_FACTORY_H

#ifndef PIPELINE_PIPELINE_CACHE_H
#include "PipelineCache.h"
#endif

// Define inlined functions that use PipelineCache (see https://stackoverflow.com/a/71470522/1487069).
#ifndef PIPELINE_PIPELINE_FACTORY_H_definitions
#define PIPELINE_PIPELINE_FACTORY_H_definitions

namespace task {

boost::intrusive_ptr<PipelineCache> PipelineFactory::detach_pipeline_cache_task()
{
  return std::move(m_pipeline_cache_task);
}

} // namespace task

#endif // PIPELINE_PIPELINE_FACTORY_H_definitions
