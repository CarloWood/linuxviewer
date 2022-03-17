#include "sys.h"
#include "SynchronousTask.h"
#include "SynchronousWindow.h"

namespace task {

char const* SynchronousTask::state_str_impl(state_type run_state) const
{
  switch(run_state)
  {
    AI_CASE_RETURN(SynchronousTask_start);
  }
  ASSERT(false);
  return "UNKNOWN STATE";
}

void SynchronousTask::initialize_impl()
{
  // If there is a derived class immediately start with its first state; otherwise start with our first state.
  set_state(typeid(SynchronousTask) == typeid(*this) ? SynchronousTask_start : state_end);
}

void SynchronousTask::multiplex_impl(state_type run_state)
{
  // We're not derived from: just call the callback and we're done.
  finish();
}

void SynchronousTask::yield_frame(unsigned int frames)
{
  AIStatefulTask::yield_frame(m_owner, frames);
}

void SynchronousTask::yield_ms(unsigned int ms)
{
  AIStatefulTask::yield_ms(m_owner, ms);
}

void SynchronousTask::run()
{
  AIStatefulTask::run(m_owner);
  m_owner->set_have_synchronous_task({});
}

void SynchronousTask::run(std::function<void (bool)> cb_function)
{
  AIStatefulTask::run(m_owner, cb_function);
  m_owner->set_have_synchronous_task({});
}

void SynchronousTask::run(AIStatefulTask* parent, condition_type condition, on_abort_st on_abort)
{
  AIStatefulTask::run(m_owner, parent, condition, on_abort);
  m_owner->set_have_synchronous_task({});
}

} // namespace task
