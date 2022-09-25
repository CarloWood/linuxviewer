#ifndef PIPELINE_PIPELINE_FACTORY_H
#define PIPELINE_PIPELINE_FACTORY_H

#include "CharacteristicRange.h"
#include "Pipeline.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/RunningTasksTracker.h"
#include "utils/MultiLoop.h"
#include "utils/Vector.h"
#include "utils/Badge.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {
class LogicalDevice;

namespace pipeline {
class CharacteristicRange;
} // namespace pipeline

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

  static constexpr condition_type pipeline_cache_set_up = 1;
  static constexpr condition_type fully_initialized = 2;

 private:
  // Constructor.
  SynchronousWindow* m_owning_window;
  vk::RenderPass m_vh_render_pass;
  // add.
  characteristics_container_t m_characteristics;

  // run
  // initialize_impl.
  statefultask::RunningTasksTracker::index_type m_index;
  // State PipelineFactory_start.
  boost::intrusive_ptr<PipelineCache> m_pipeline_cache_task;
  // State PipelineFactory_initialized.
  vulkan::pipeline::FlatCreateInfo m_flat_create_info;
  vulkan::pipeline::ShaderInputData m_shader_input_data;
  MultiLoop m_range_counters;
  int m_start_of_next_loop;
  boost::intrusive_ptr<synchronous::MoveNewPipelines> m_move_new_pipelines_synchronously;
  // State MoveNewPipelines_need_action (which calls set_pipeline).
  vulkan::Pipeline& m_pipeline_out;
  // Index into SynchronousWindow::m_pipeline_factories, pointing to ourselves.
  PipelineFactoryIndex m_pipeline_factory_index;
  // Index into SynchronousWindow::m_pipelines, enumerating the current pipeline being generated inside the MultiLoop.
  vulkan::pipeline::Index m_pipeline_index;
  // Layout of the current pipeline that is being created inside the MultiLoop.
  vk::PipelineLayout m_vh_pipeline_layout;

 protected:
  using direct_base_type = AIStatefulTask;      // The immediate base class of this task.

  // The different states of the task.
  enum PipelineFactory_state_type {
    PipelineFactory_start = direct_base_type::state_end,
    PipelineFactory_initialize,
    PipelineFactory_initialized,
    PipelineFactory_top_multiloop_for_loop,
    PipelineFactory_top_multiloop_while_loop,
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

 public:
  PipelineFactory(SynchronousWindow* owning_window, vulkan::Pipeline& pipeline_out, vk::RenderPass vh_render_pass
      COMMA_CWDEBUG_ONLY(bool debug = false));

  // Accessor.
  SynchronousWindow* owning_window() const { return m_owning_window; }

  void add(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
  void set_index(PipelineFactoryIndex pipeline_factory_index) { m_pipeline_factory_index = pipeline_factory_index; }
  void set_pipeline(vulkan::Pipeline&& pipeline) { m_pipeline_out = std::move(pipeline); }
  characteristics_container_t const& characteristics() const { return m_characteristics; }

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
