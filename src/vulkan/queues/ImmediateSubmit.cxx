#include "sys.h"
#include "ImmediateSubmit.h"
#include "ImmediateSubmitQueue.h"
#include "Exceptions.h"
#include "Application.h"
#include "QueuePool.h"
#include "utils/AIAlert.h"

namespace vulkan::task {

ImmediateSubmit::ImmediateSubmit(CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug))
{
}

ImmediateSubmit::ImmediateSubmit(ImmediateSubmitRequest&& submit_request, state_type continue_state COMMA_CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug)), m_submit_request(std::move(submit_request)), m_continue_state(continue_state)
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmit(" << submit_request << ", " << continue_state << ") [" << this << "]");
}

ImmediateSubmit::~ImmediateSubmit()
{
  DoutEntering(dc::statefultask(mSMDebug), "~ImmediateSubmit() [" << this << "]");
}

char const* ImmediateSubmit::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(submit_finished);
  }
  return direct_base_type::condition_str_impl(condition);
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

char const* ImmediateSubmit::task_name_impl() const
{
  return "ImmediateSubmit";
}

void ImmediateSubmit::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmit_start:
    {
      // Obtain reference to associated QueuePool.
      QueuePool& queue_pool = QueuePool::instance(m_submit_request);
      // Get a running ImmediateSubmitQueue task from the pool.
      m_immediate_submit_queue_task = queue_pool.get_immediate_submit_queue_task(CWDEBUG_ONLY(mSMDebug));

      // Pass on the submit request.
      m_immediate_submit_queue_task->have_new_datum(std::move(m_submit_request));
      set_state(m_continue_state);
      wait(submit_finished);
      break;
    }
    case ImmediateSubmit_done:
      finish();
      break;
  }
}

} // namespace vulkan::task
