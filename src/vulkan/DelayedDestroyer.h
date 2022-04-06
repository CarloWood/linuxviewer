#pragma once

#include "SynchronousTask.h"
#include <vulkan/vulkan.hpp>

namespace task::synchronous {

class DelayedDestroyer : public SynchronousTask
{
 protected:
  /// The base class of this task.
  using direct_base_type = SynchronousTask;

  /// The different states of the stateful task.
  enum delayed_destroyer_task_state_type {
    DelayedDestroyer_yield = direct_base_type::state_end,
    DelayedDestroyer_done
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = DelayedDestroyer_done + 1;

 private:
  vk::UniqueSemaphore m_kept_semaphore;         // Semaphore to keep alive for m_frames frames.
  int m_frames;                                 // Delay in frames.

 public:
  DelayedDestroyer(vk::UniqueSemaphore&& semaphore, SynchronousWindow* owning_window, int delay COMMA_CWDEBUG_ONLY(bool debug)) :
    SynchronousTask(owning_window COMMA_CWDEBUG_ONLY(debug)), m_kept_semaphore(std::move(semaphore)), m_frames(delay)
  {
    DoutEntering(dc::vulkan, "DelayedDestroyer::DelayedDestroyer(" << *semaphore << ", " << owning_window << ", " << delay << ") [" << this << "]")
  }

 protected:
  /// Call finish() (or abort()), not delete.
  ~DelayedDestroyer() override
  {
    DoutEntering(dc::vulkan, "DelayedDestroyer::~DelayedDestroyer() [" << this << "]");
    Dout(dc::vulkan, "Destroying " << *m_kept_semaphore);
  }

  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Set the start state to DelayedDestroyer_start.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override final;
};

} // namespace task::synchronous
