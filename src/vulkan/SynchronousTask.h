#pragma once

#include "SynchronousWindow.h"
#include "statefultask/AIStatefulTask.h"

namespace task {

// Tasks that will be executed from the render loop, synchronous with
// (not at the same time as) drawing the frame (the call to draw_frame()).
class SynchronousTask : public AIStatefulTask
{
 private:
  using AIStatefulTask::target;

 private:
  SynchronousWindow* m_owner;                // The SynchronousWindow that this object is a member of.

 public:
  // Constructor.
  SynchronousTask(SynchronousWindow* owner COMMA_CWDEBUG_ONLY(bool debug = false)) : AIStatefulTask(CWDEBUG_ONLY(debug)), m_owner(owner) { }

 protected:
  /// The base class of this task.
  using direct_base_type = AIStatefulTask;

  // Allow only yielding to the same engine.
  void yield() { AIStatefulTask::yield(); }
  void yield_frame(unsigned int frames) { AIStatefulTask::yield_frame(m_owner, frames); }
  void yield_ms(unsigned int ms) { AIStatefulTask::yield_ms(m_owner, ms); }

  /// The different states of the stateful task.
  enum synchronous_task_state_type {
    SynchronousTask_start = direct_base_type::state_end,
  };

 public:
  /// One beyond the largest state of this task.
  static constexpr state_type state_end = SynchronousTask_start + 1;

  // Allow only running in the provided engine.
  void run()
  {
    AIStatefulTask::run(m_owner);
    m_owner->set_have_synchronous_task({});
  }

  void run(std::function<void (bool)> cb_function)
  {
    AIStatefulTask::run(m_owner, cb_function);
    m_owner->set_have_synchronous_task({});
  }

  void run(AIStatefulTask* parent, condition_type condition, on_abort_st on_abort = abort_parent)
  {
    AIStatefulTask::run(m_owner, parent, condition, on_abort);
    m_owner->set_have_synchronous_task({});
  }

 protected:
  /// Implemenation of state_str for run states.
  char const* state_str_impl(state_type run_state) const override;

  /// Run bs_initialize.
  void initialize_impl() override;

  /// Handle mRunState.
  void multiplex_impl(state_type run_state) override;
};

} // namespace task
