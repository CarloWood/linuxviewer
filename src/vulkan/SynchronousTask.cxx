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

void SynchronousTask::multiplex_impl(state_type run_state)
{
  // We should be starting directly with the state of the derived class.
  DoutEntering(dc::warning, "Running SynchronousTask::multiplex_impl");

  // This engine isn't really doing anything. Just continue with the derived task.
  set_state(state_end);
}

} // namespace task
