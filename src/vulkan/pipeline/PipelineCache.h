#pragma once

#include "CacheData.h"
#include "vk_utils/TaskToTaskDeque.h"
#include "statefultask/AIStatefulTask.h"
#include "threadsafe/aithreadsafe.h"
#include "utils/nearest_multiple_of_power_of_two.h"
#include "debug.h"
#include "debug/DebugSetName.h"
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vulkan/vulkan.hpp>
#include <filesystem>
#include <cstdlib>

namespace task {

class PipelineCache : public vk_utils::TaskToTaskDeque<AIStatefulTask, int>, public vulkan::pipeline::CacheData
{
  static constexpr condition_type condition_flush_to_disk = 2;

 private:
  vk::UniquePipelineCache m_pipeline_cache;

#ifdef CWDEBUG
  vulkan::Ambifix m_load_ambifix;
#endif

 protected:
  // The different states of the stateful task.
  enum PipelineCache_state_type {
    PipelineCache_initialize = direct_base_type::state_end,
    PipelineCache_load_from_disk,
    PipelineCache_ready,
    PipelineCache_save_to_disk,
    PipelineCache_done,
  };

 public:
  // One beyond the largest state of this task.
  static constexpr state_type state_end = PipelineCache_done + 1;

 protected:
  // boost::serialization::access has a destroy function that want to call operator delete on us. Of course, we will never call destroy.
  friend class boost::serialization::access;
  ~PipelineCache() override                     // Call finish() (or abort()), not delete.
  {
    DoutEntering(dc::statefultask(mSMDebug), "~PipelineCache() [" << (void*)this << "]");
  }

  // Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  // Handle mRunState.
  void multiplex_impl(state_type run_state) override;

 public:
  PipelineCache(CWDEBUG_ONLY(bool debug = false)) : direct_base_type(CWDEBUG_ONLY(debug)) { }

  std::filesystem::path get_filename() const;

  void clear_cache();

  void load(boost::archive::binary_iarchive& archive, unsigned int const version);
  void save(boost::archive::binary_oarchive& archive, unsigned int const version) const;

  // Accessor for the create pipeline cache.
  vk::PipelineCache vh_pipeline_cache() const { return *m_pipeline_cache; }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

} // namespace task
