#pragma once

#include "PipelineCache.h"
#include "CharacteristicRange.h"
#include "statefultask/AIStatefulTask.h"
#include "utils/MultiLoop.h"
#include "utils/Vector.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {
class LogicalDevice;

namespace pipeline {
class CharacteristicRange;
} // namespace pipeline

} // namespace vulkan

namespace task {

namespace synchronous {
class MoveNewPipelines;
class CreateVertexBuffers;
} // namespace synchronous

class PipelineFactory : public AIStatefulTask
{
 public:
  using PipelineFactoryIndex = utils::VectorIndex<boost::intrusive_ptr<PipelineFactory>>;

  static constexpr condition_type pipeline_cache_set_up = 1;
  static constexpr condition_type fully_initialized = 2;

 private:
  // Constructor.
  SynchronousWindow* m_owning_window;
  vk::PipelineLayout m_vh_pipeline_layout;
  vk::RenderPass m_vh_render_pass;
  // add.
  std::vector<boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange>> m_characteristics;

  // run
  // State PipelineFactory_start.
  boost::intrusive_ptr<PipelineCache> m_pipeline_cache_task;
  // State PipelineFactory_initialized.
  vulkan::pipeline::FlatCreateInfo m_flat_create_info;
  MultiLoop m_range_counters;
  boost::intrusive_ptr<synchronous::MoveNewPipelines> m_move_new_pipelines_synchronously;
  boost::intrusive_ptr<synchronous::CreateVertexBuffers> m_create_vertex_buffers_synchronously;
  // Index into SynchronousWindow::m_pipeline_factories, pointing to ourselves.
  PipelineFactoryIndex m_pipeline_factory_index;

 protected:
  using direct_base_type = AIStatefulTask;      // The immediate base class of this task.

  // The different states of the task.
  enum PipelineFactory_state_type {
    PipelineFactory_start = direct_base_type::state_end,
    PipelineFactory_initialize,
    PipelineFactory_initialized,
    PipelineFactory_generate,
    PipelineFactory_done
  };

 public:
  static state_type constexpr state_end = PipelineFactory_done + 1;

 protected:
  ~PipelineFactory() override;

  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;

 public:
  PipelineFactory(SynchronousWindow* owning_window, vk::PipelineLayout vh_pipeline_layout, vk::RenderPass vh_render_pass
      COMMA_CWDEBUG_ONLY(bool debug = false));

  // Accessor.
  SynchronousWindow const* owning_window() const { return m_owning_window; }

  void add(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
  void set_index(PipelineFactoryIndex pipeline_factory_index) { m_pipeline_factory_index = pipeline_factory_index; }

  // Called by SynchronousWindow::pipeline_factory_done to rescue the cache, immediately before deleting this task.
  boost::intrusive_ptr<PipelineCache> detach_pipeline_cache_task() { return std::move(m_pipeline_cache_task); }
};

} // namespace task
