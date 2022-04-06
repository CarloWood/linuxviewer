#include "sys.h"
#include "DelayedDestroyer.h"

namespace task::synchronous {

char const* DelayedDestroyer::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(DelayedDestroyer_yield);
    AI_CASE_RETURN(DelayedDestroyer_done);
  }
  if (AI_UNLIKELY(run_state >= direct_base_type::state_end))
    return "UNKNOWN STATE";
  return direct_base_type::state_str_impl(run_state);
}

void DelayedDestroyer::initialize_impl()
{
  DoutEntering(dc::statefultask(mSMDebug), "DelayedDestroyer::initialize_impl() [" << (void*)this << "]");
  set_state(DelayedDestroyer_yield);
}

void DelayedDestroyer::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case DelayedDestroyer_yield:
      set_state(DelayedDestroyer_done);
      // Yield m_frames frames long.
      yield_frame(m_frames);
      break;
    case DelayedDestroyer_done:
      finish();
      break;
    default:
      // Run base class states (this is not used because we set the state to DelayedDestroyer_yield in initialize_impl).
      ASSERT(run_state < DelayedDestroyer_yield);
      SynchronousTask::multiplex_impl(run_state);
      break;
  }
}

} // namespace task::synchronous
