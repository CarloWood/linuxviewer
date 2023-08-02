#pragma once

#ifndef PIPELINE_PIPELINE_CACHE_H
#define PIPELINE_PIPELINE_CACHE_H

#include "../vk_utils/TaskToTaskDeque.h"
#include "statefultask/AIStatefulTask.h"
#include "threadsafe/threadsafe.h"
#include "utils/nearest_multiple_of_power_of_two.h"
#include "utils/ulong_to_base.h"
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <cstdlib>
#ifdef CWDEBUG
#include "../debug/DebugSetName.h"
#endif
#include "debug.h"

namespace vulkan::task {

class PipelineFactory;

class PipelineCache : public vk_utils::TaskToTaskDeque<AIStatefulTask, vk::UniquePipelineCache> // Other PipelineCache tasks can pass their pipeline cache for merging.
{
 public:
  static constexpr condition_type condition_flush_to_disk = 2;
  static constexpr condition_type factory_finished = 4;

 private:
  // Constructor.
  boost::intrusive_ptr<PipelineFactory> m_owning_factory; // We have one pipeline cache per factory - or each factory would still be
                                                                // slowed down as a result of concurrent accesses to the cache.
                                                                // This is a intrusive_ptr because the PipelineFactory might finish before this task.
  // State PipelineCache_load_from_disk.
  vk::UniquePipelineCache m_pipeline_cache;

  bool m_is_merger = false;

 protected:
  // The different states of the stateful task.
  enum PipelineCache_state_type {
    PipelineCache_initialize = direct_base_type::state_end,
    PipelineCache_load_from_disk,
    PipelineCache_ready,
    PipelineCache_factory_finished,
    PipelineCache_factory_merge,
    PipelineCache_save_to_disk,
    PipelineCache_done,
  };

 public:
  // One beyond the largest state of this task.
  static constexpr state_type state_end = PipelineCache_done + 1;

  // Called by Application::pipeline_factory_done.
  void set_is_merger() { m_is_merger = true; }

 protected:
  // boost::serialization::access has a destroy function that want to call operator delete on us. Of course, we will never call destroy.
  friend class boost::serialization::access;
  ~PipelineCache() override;                    // Call finish(), not delete.

  // Implementation of virtual functions of AIStatefulTask.
  char const* condition_str_impl(condition_type condition) const override;
  char const* state_str_impl(state_type run_state) const override;
  char const* task_name_impl() const override;
  void multiplex_impl(state_type run_state) override;

#ifdef CWDEBUG
  void abort_impl() final
  {
    // A PipelineCache should not be aborted; it should write to disk at program termination.
    ASSERT(false);
  }
#endif

 public:
  PipelineCache(PipelineFactory* factory COMMA_CWDEBUG_ONLY(bool debug = false));

  std::filesystem::path get_filename() const;

  void clear_cache();

  void load(boost::archive::binary_iarchive& archive, unsigned int const version);
  void save(boost::archive::binary_oarchive& archive, unsigned int const version) const;

  // Accessor for the create pipeline cache.
  vk::PipelineCache vh_pipeline_cache() const { return *m_pipeline_cache; }

  // Rescue pipeline cache just before deleting this task. Called by Application::pipeline_factory_done.
  vk::UniquePipelineCache detach_pipeline_cache() { return std::move(m_pipeline_cache); }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace vulkan::task

#endif // PIPELINE_PIPELINE_CACHE_H
