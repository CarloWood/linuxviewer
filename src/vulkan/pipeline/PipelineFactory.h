#ifndef PIPELINE_PIPELINE_FACTORY_H
#define PIPELINE_PIPELINE_FACTORY_H

#include "FlatCreateInfo.h"
#include "../Pipeline.h"
#include "ShaderResourcePlusCharacteristic.h"
#include "ShaderResourcePlusCharacteristicIndex.h"
#include "../descriptor/SetKeyPreference.h"
#include "../descriptor/SetKeyToShaderResourceDeclaration.h"
#include "../shader_builder/shader_resource/UniformBuffer.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/RunningTasksTracker.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/MultiLoop.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {
class LogicalDevice;

namespace task {
class CombinedImageSamplerUpdater;
class PipelineCache;
class CharacteristicRange;
} // namespace task

namespace pipeline {
class FactoryCharacteristicId;
class AddShaderStage;
using CharacteristicRangeIndex = utils::VectorIndex<task::CharacteristicRange>;
} // namespace pipeline

namespace shader_builder::shader_resource {
class CombinedImageSampler;
} // namespace shader_builder::shader_resource

namespace task {

namespace synchronous {
class MoveNewPipelines;
} // namespace synchronous

class PipelineFactory : public AIStatefulTask
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<PipelineFactory>>;
  using characteristics_container_t = utils::Vector<boost::intrusive_ptr<CharacteristicRange>, pipeline::CharacteristicRangeIndex>;
  // The same as CharacteristicRange::pipeline_index_t.
  using pipeline_index_t = aithreadsafe::Wrapper<pipeline::Index, aithreadsafe::policy::Primitive<std::mutex>>;

  static constexpr condition_type pipeline_cache_set_up = 0x1;
  static constexpr condition_type fully_initialized = 0x2;
  static constexpr condition_type characteristics_initialized = 0x4;
  static constexpr condition_type characteristics_filled = 0x8;
  static constexpr condition_type characteristics_preprocessed = 0x10;
  static constexpr condition_type characteristics_compiled = 0x20;
  static constexpr condition_type obtained_create_lock = 0x40;
  static constexpr condition_type obtained_set_layout_binding_lock = 0x80;
  static constexpr condition_type combined_image_samplers_updated = 0x100;

 private:
  // Constructor.
  SynchronousWindow* m_owning_window;
  vk::RenderPass m_vh_render_pass;
  // add.
  characteristics_container_t m_characteristics;
  // Index into SynchronousWindow::m_pipeline_factories, pointing to ourselves.
  PipelineFactoryIndex m_pipeline_factory_index;

  // run
  // initialize_impl.
  statefultask::RunningTasksTracker::index_type m_index;
  // State PipelineFactory_start.
  boost::intrusive_ptr<PipelineCache> m_pipeline_cache_task;
  // State PipelineFactory_initialize.
  boost::intrusive_ptr<synchronous::MoveNewPipelines> m_move_new_pipelines_synchronously;
  // State PipelineFactory_initialized.
  pipeline::FlatCreateInfo m_flat_create_info;
  utils::Vector<unsigned int, pipeline::CharacteristicRangeIndex> m_range_shift;
  MultiLoop m_range_counters;
#if CW_DEBUG
  bool m_debug_reached_characteristics_initialized;     // Set to true when PipelineFactory_characteristics_initialized was reached.
#endif
  int m_start_of_next_loop;
  // Keep track of tasks that we have to wait for (to do a callback when they finish their current state).
  std::atomic<size_t> m_number_of_running_characteristic_tasks;
  // A bit mask of tasks that already completed all fill_index values at least once.
  uint64_t m_completed_characteristic_tasks;
  // A bit mask of tasks that did ran the fill state because their fill_index changed.
  uint64_t m_running_characteristic_tasks;
  static uint64_t to_bit_mask(pipeline::CharacteristicRangeIndex index) { return uint64_t{1} << index.get_value(); }
  // State PipelineFactory_top_multiloop_while_loop
  descriptor::SetIndexHintMap m_set_index_hint_map;
  // PipelineFactory_characteristics_preprocessed.
  descriptor::SetIndexHint m_largest_set_index_hint;
  // Keep track of the number of combined image samplers that need updating before starting to render to the next pipeline.
  std::atomic<size_t> m_number_of_running_combined_image_sampler_updaters;
  // Temporary storage of a newly created pipeline before it is moved to the SynchronousWindow.
  vk::UniquePipeline m_pipeline;
  // State MoveNewPipelines_need_action (which calls set_pipeline).
  Pipeline& m_pipeline_out;
  // Index into SynchronousWindow::m_pipelines, enumerating the current pipeline being generated inside the MultiLoop.
  pipeline_index_t m_pipeline_index;
  // Layout of the current pipeline that is being created inside the MultiLoop.
  vk::PipelineLayout m_vh_pipeline_layout;
  // Set to true when calling update_missing_descriptor_sets while already having the set_layout_binding lock for the current pipeline/set_index/first_shader_resource.
  bool m_have_lock;

  struct ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences
  {
    // List of shader resource / Characteristic pairs (range plus range value), where the latter added the former from the add_* functions like add_uniform_buffer.
    pipeline::ShaderResourcePlusCharacteristic m_shader_resource_plus_characteristic;
    // Corresponding preferred and undesirable descriptor sets.
    std::vector<descriptor::SetKeyPreference> m_preferred_descriptor_sets;
    std::vector<descriptor::SetKeyPreference> m_undesirable_descriptor_sets;
    // Mutable because I prefer to only keep the read-lock on this object when preparing.
    // Writing to this boolean is thread-safe because it is down "single-threaded":
    // by PipelineFactory_characteristics_filled.
    mutable bool m_prepared{false};

#ifdef CWDEBUG
    void print_on(std::ostream& os) const;
#endif
  };
  using added_shader_resource_plus_characteristic_list_container_t = utils::Vector<ShaderResourcePlusCharacteristicPlusPreferredAndUndesirableSetKeyPreferences, pipeline::ShaderResourcePlusCharacteristicIndex>;
  using added_shader_resource_plus_characteristic_list_t = aithreadsafe::Wrapper<added_shader_resource_plus_characteristic_list_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  added_shader_resource_plus_characteristic_list_t m_added_shader_resource_plus_characteristic_list;

  bool test_and_set_prepared(pipeline::ShaderResourcePlusCharacteristicIndex index, added_shader_resource_plus_characteristic_list_t::rat const& added_shader_resource_plus_characteristic_list_r)
  {
    bool is_prepared = added_shader_resource_plus_characteristic_list_r->operator[](index).m_prepared;
    if (!is_prepared)
      added_shader_resource_plus_characteristic_list_r->operator[](index).m_prepared = true;
    return is_prepared;
  }

  //---------------------------------------------------------------------------
  // Shader resources.

  // Set index hints.
  using set_key_to_set_index_hint_container_t = std::map<descriptor::SetKey, descriptor::SetIndexHint>;
  set_key_to_set_index_hint_container_t m_set_key_to_set_index_hint;     // Maps descriptor::SetKey's to descriptor::SetIndexHint's.
  using glsl_id_to_shader_resource_container_t = std::map<std::string, shader_builder::ShaderResourceDeclaration, std::less<>>;
  using glsl_id_to_shader_resource_t = aithreadsafe::Wrapper<glsl_id_to_shader_resource_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  glsl_id_to_shader_resource_t m_glsl_id_to_shader_resource;  // Set index hint needs to be updated afterwards.
  descriptor::SetKeyToShaderResourceDeclaration m_shader_resource_set_key_to_shader_resource_declaration;
     // Maps descriptor::SetKey's to shader resource declaration object used for the current pipeline.
     // Note: write access to this member is hijacking the write-lock on m_glsl_id_to_shader_resource for concurrent access protection.
     // It is read in update_missing_descriptor_sets and allocate_update_add_handles_and_unlocking which doesn't require any locking
     // because nobody is writing to it anymore by then.

  using sorted_descriptor_set_layouts_container_t = std::vector<descriptor::SetLayout>;
  using sorted_descriptor_set_layouts_t = aithreadsafe::Wrapper<sorted_descriptor_set_layouts_container_t, aithreadsafe::policy::Primitive<std::mutex>>;
  sorted_descriptor_set_layouts_t m_sorted_descriptor_set_layouts;      // A Vector of SetLayout objects, containing vk::DescriptorSetLayout
                                                                        // handles and the vk::DescriptorSetLayoutBinding objects, stored in
                                                                        // a sorted vector, that they were created from.

  // Initialized in handle_shader_resource_creation_requests:
  // The list of shader resources that we managed to get the lock on and weren't already created before.
  std::vector<shader_builder::ShaderResourceBase*> m_acquired_required_shader_resources_list;
  // Initialized in initialize_shader_resources_per_set_index.
  utils::Vector<std::vector<pipeline::ShaderResourcePlusCharacteristic>, descriptor::SetIndex> m_added_shader_resource_plus_characteristics_per_used_set_index;
  // The largest set_index used in m_added_shader_resource_plus_characteristic_list, plus one.
  descriptor::SetIndex m_set_index_end;
  // The current descriptor set index that we're processing in update_missing_descriptor_sets.
  descriptor::SetIndex m_set_index;

  // Descriptor set handles, sorted by descriptor set index, required for the next pipeline.
  Pipeline::descriptor_set_per_set_index_t m_descriptor_set_per_set_index;

  //---------------------------------------------------------------------------

 private:
  // Called from prepare_shader_resource_declarations.
  void fill_set_index_hints(added_shader_resource_plus_characteristic_list_t::rat const& added_shader_resource_plus_characteristic_list_r, utils::Vector<descriptor::SetIndexHint, pipeline::ShaderResourcePlusCharacteristicIndex>& set_index_hints_out);
  void prepare_shader_resource_declarations();

  // Returns the SetIndexHint that was assigned to this key (usually by shader_resource::add_*).
  descriptor::SetIndexHint get_set_index_hint(descriptor::SetKey set_key) const
  {
    DoutEntering(dc::setindexhint(mSMDebug)|continued_cf, "PipelineFactory::get_set_index_hint(" << set_key << ") = ");
    auto set_index_hint = m_set_key_to_set_index_hint.find(set_key);
    // Don't call get_set_index_hint for a key that wasn't added yet.
    ASSERT(set_index_hint != m_set_key_to_set_index_hint.end());
    descriptor::SetIndexHint result = set_index_hint->second;
    Dout(dc::finish, result << " [" << this << "]");
    return result;
  }

  shader_builder::ShaderResourceDeclaration const* get_declaration(descriptor::SetKey set_key) const
  {
    return m_shader_resource_set_key_to_shader_resource_declaration.get_shader_resource_declaration(set_key);
  }

 public:
  //---------------------------------------------------------------------------
  // Shader resources.

  void add_combined_image_sampler(utils::Badge<CharacteristicRange>,
      shader_builder::shader_resource::CombinedImageSampler const& combined_image_sampler,
      CharacteristicRange* adding_characteristic_range,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  void add_uniform_buffer(utils::Badge<CharacteristicRange>,
      shader_builder::UniformBufferBase const& uniform_buffer,
      CharacteristicRange* adding_characteristic_range,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets = {},
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets = {});

  shader_builder::ShaderResourceDeclaration* realize_shader_resource_declaration(utils::Badge<CharacteristicRange>, std::string glsl_id_full, vk::DescriptorType descriptor_type, shader_builder::ShaderResourceBase const& shader_resource, descriptor::SetIndexHint set_index_hint);

  void realize_descriptor_set_layouts(utils::Badge<pipeline::AddShaderStage>);

  void push_back_descriptor_set_layout_binding(descriptor::SetIndexHint set_index_hint,
      vk::DescriptorSetLayoutBinding const& descriptor_set_layout_binding,
      vk::DescriptorBindingFlags binding_flags, int32_t descriptor_array_size,
      utils::Badge<shader_builder::DescriptorSetLayoutBinding>);

 private:
  // Called by add_combined_image_sampler and/or add_uniform_buffer (at the end), requesting to be created
  // and storing the preferred and undesirable descriptor set vectors.
  void add_shader_resource(shader_builder::ShaderResourceBase const* shader_resource,
      CharacteristicRange* adding_characteristic_range,
      std::vector<descriptor::SetKeyPreference> const& preferred_descriptor_sets,
      std::vector<descriptor::SetKeyPreference> const& undesirable_descriptor_sets);

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
      std::vector<std::pair<descriptor::SetIndex, bool>> const& set_index_has_frame_resource_pairs,
      descriptor::SetIndex set_index_begin, descriptor::SetIndex set_index_end);

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
    PipelineFactory_move_new_pipeline,
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
  PipelineFactory(SynchronousWindow* owning_window, Pipeline& pipeline_out, vk::RenderPass vh_render_pass
      COMMA_CWDEBUG_ONLY(bool debug = false));

  // Accessor.
  SynchronousWindow* owning_window() const { return m_owning_window; }

  template<typename T>
  void add_to_flat_create_info(std::vector<T> const* list, task::CharacteristicRange const& owning_characteristic_range)
  {
    m_flat_create_info.add(list, owning_characteristic_range);
  }

  pipeline::FactoryCharacteristicId add_characteristic(boost::intrusive_ptr<CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
  void set_index(PipelineFactoryIndex pipeline_factory_index) { m_pipeline_factory_index = pipeline_factory_index; }
  void set_pipeline(Pipeline&& pipeline) { m_pipeline_out = std::move(pipeline); }
  characteristics_container_t const& characteristics() const { return m_characteristics; }
  PipelineFactoryIndex pipeline_factory_index() const { return m_pipeline_factory_index; }

  // Called from CharacteristicRange::multiplex_impl.
  void characteristic_range_initialized();
  void characteristic_range_filled();
  void characteristic_range_preprocessed();
  void characteristic_range_compiled();
  void descriptor_set_update_start();
  void descriptor_set_updated();

  // Called by SynchronousWindow::pipeline_factory_done to rescue the cache, immediately before deleting this task.
  inline boost::intrusive_ptr<PipelineCache> detach_pipeline_cache_task();
};

} // namespace task
} // namespace vulkan
#endif // PIPELINE_PIPELINE_FACTORY_H

#ifndef PIPELINE_PIPELINE_CACHE_H
#include "PipelineCache.h"
#endif

// Define inlined functions that use PipelineCache (see https://stackoverflow.com/a/71470522/1487069).
#ifndef PIPELINE_PIPELINE_FACTORY_H_definitions
#define PIPELINE_PIPELINE_FACTORY_H_definitions

namespace vulkan::task {

boost::intrusive_ptr<PipelineCache> PipelineFactory::detach_pipeline_cache_task()
{
  return std::move(m_pipeline_cache_task);
}

} // namespace vulkan::task

#endif // PIPELINE_PIPELINE_FACTORY_H_definitions
