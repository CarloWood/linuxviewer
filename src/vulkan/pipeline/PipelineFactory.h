#ifndef PIPELINE_PIPELINE_FACTORY_H
#define PIPELINE_PIPELINE_FACTORY_H

#include "FlatCreateInfo.h"
#include "ShaderInputData.h"
#include "Pipeline.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/RunningTasksTracker.h"
#include "utils/MultiLoop.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {
class LogicalDevice;
} // namespace vulkan

namespace vulkan::pipeline {
class FactoryRangeId;
} // namespace vulkan::pipeline

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
  static constexpr condition_type characteristics_compiled = 16;
  static constexpr condition_type obtained_create_lock = 32;
  static constexpr condition_type obtained_set_layout_binding_lock = 64;

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
  vulkan::pipeline::FlatCreateInfo m_flat_create_info;
  vulkan::pipeline::ShaderInputData m_shader_input_data;
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
  // Set to true when calling ShaderInputData::update_missing_descriptor_sets while already having the set_layout_binding lock for the current pipeline/set_index/first_shader_resource.
  bool m_have_lock;

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

  vulkan::pipeline::FactoryRangeId add_characteristic(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
  void set_index(PipelineFactoryIndex pipeline_factory_index) { m_pipeline_factory_index = pipeline_factory_index; }
  void set_pipeline(vulkan::Pipeline&& pipeline) { m_pipeline_out = std::move(pipeline); }
  characteristics_container_t const& characteristics() const { return m_characteristics; }
  PipelineFactoryIndex pipeline_factory_index() const { return m_pipeline_factory_index; }

  // Called from CharacteristicRange::multiplex_impl.
  void characteristic_range_initialized();
  void characteristic_range_filled(vulkan::pipeline::CharacteristicRangeIndex index);
  void characteristic_range_compiled();

  void added_creation_request(vulkan::pipeline::ShaderInputData const* shader_input_data);
  // Give pipeline::CharacteristicRange read/write access to m_shader_input_data.
  vulkan::pipeline::ShaderInputData const& shader_input_data(utils::Badge<vulkan::pipeline::CharacteristicRange>) const { return m_shader_input_data; }
  vulkan::pipeline::ShaderInputData& shader_input_data(utils::Badge<vulkan::pipeline::CharacteristicRange>) { return m_shader_input_data; }

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
