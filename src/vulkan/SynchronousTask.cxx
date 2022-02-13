#include "sys.h"
#include "SynchronousTask.h"

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

} // namespace task
