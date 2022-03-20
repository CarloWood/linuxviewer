#pragma once

#include "PipelineCache.h"
#include "CharacteristicRange.h"
#include "SynchronousTask.h"
#include "statefultask/AIStatefulTask.h"
#include "statefultask/Broker.h"
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

class PipelineFactory : public AIStatefulTask
{
  using pipeline_cache_broker_type = task::Broker<PipelineCache>;

  static constexpr condition_type pipeline_cache_set_up = 1;
  static constexpr condition_type fully_initialized = 2;

 private:
  // Constructor.
  SynchronousWindow* m_owning_window;
  vk::PipelineLayout m_vh_pipeline_layout;
  vk::RenderPass m_vh_render_pass;
  // set_pipeline_cache_broker
  boost::intrusive_ptr<pipeline_cache_broker_type> m_broker;
  // add.
  std::vector<boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange>> m_characteristics;

  // run
  // State PipelineFactory_start.
  boost::intrusive_ptr<PipelineCache const> m_pipeline_cache_task;
  // State PipelineFactory_initialized.
  vulkan::pipeline::FlatCreateInfo m_flat_create_info;
  MultiLoop m_range_counters;
  boost::intrusive_ptr<SynchronousTask> m_finished_watcher;             // This task just waits - synchronously - until the PipelineFactory finished.
  // State PipelineFactory_generate.
  utils::Vector<vk::UniquePipeline, vulkan::pipeline::Index> m_graphics_pipelines;

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
  ~PipelineFactory() override
  {
    DoutEntering(dc::statefultask(mSMDebug), "~PipelineFactory() [" << this << "]");
  }

  char const* state_str_impl(state_type run_state) const override;
  void multiplex_impl(state_type run_state) override;

 public:
  PipelineFactory(SynchronousWindow* owning_window, vk::PipelineLayout vh_pipeline_layout, vk::RenderPass vh_render_pass
      COMMA_CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug)),
      m_owning_window(owning_window), m_vh_pipeline_layout(vh_pipeline_layout), m_vh_render_pass(vh_render_pass) { }

  void set_pipeline_cache_broker(boost::intrusive_ptr<pipeline_cache_broker_type> broker);
  void add(boost::intrusive_ptr<vulkan::pipeline::CharacteristicRange> characteristic_range);
  void generate() { signal(fully_initialized); }
};

} // namespace task
