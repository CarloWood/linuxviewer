#include "sys.h"
#include "ImmediateSubmit.h"
#include "ImmediateSubmitQueue.h"
#include "Exceptions.h"
#include "Application.h"
#include "QueuePool.h"
#include "utils/AIAlert.h"

namespace task {

ImmediateSubmit::ImmediateSubmit(CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug))
{
}

ImmediateSubmit::ImmediateSubmit(vulkan::ImmediateSubmitRequest&& submit_request, state_type continue_state COMMA_CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug)), m_submit_request(std::move(submit_request)), m_continue_state(continue_state)
{
}

ImmediateSubmit::~ImmediateSubmit()
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmit::~ImmediateSubmit() [" << this << "]");
}

char const* ImmediateSubmit::state_str_impl(state_type run_state) const
{
  // If this fails then a derived class forgot to add an AI_CASE_RETURN for this state.
  ASSERT(run_state < state_end);
  switch (run_state)
  {
    AI_CASE_RETURN(ImmediateSubmit_start);
    AI_CASE_RETURN(ImmediateSubmit_done);
  }
  AI_NEVER_REACHED
}

void ImmediateSubmit::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmit_start:
    {
      // Obtain reference to associated QueuePool.
      vulkan::QueuePool& queue_pool = vulkan::QueuePool::instance(m_submit_request);
      // Get a running ImmediateSubmitQueue task from the pool.
      ImmediateSubmitQueue* immediate_submit_queue_task = queue_pool.get_immediate_submit_queue_task(CWDEBUG_ONLY(mSMDebug));

      // Pass on the submit request.
      immediate_submit_queue_task->have_new_datum(std::move(m_submit_request));
      set_state(m_continue_state);
      wait(submit_finished);
      break;
    }
    case ImmediateSubmit_done:
      finish();
      break;
  }
}

} // namespace task
