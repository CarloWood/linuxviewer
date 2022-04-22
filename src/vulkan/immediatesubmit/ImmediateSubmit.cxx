#include "sys.h"
#include "ImmediateSubmit.h"
#include "ImmediateSubmitQueue.h"
#include "Exceptions.h"
#include "Application.h"
#include "utils/AIAlert.h"

namespace task {

ImmediateSubmit::ImmediateSubmit(CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug))
{
}

ImmediateSubmit::ImmediateSubmit(vulkan::ImmediateSubmitData&& submit_data COMMA_CWDEBUG_ONLY(bool debug)) :
  AsyncTask(CWDEBUG_ONLY(debug)), m_submit_data(std::move(submit_data))
{
}

ImmediateSubmit::~ImmediateSubmit()
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmit::~ImmediateSubmit() [" << this << "]");
}

char const* ImmediateSubmit::state_str_impl(state_type run_state) const
{
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
      // Get a pointer to the task::ImmediateSubmitQueue associated with the vulkan::QueueRequestKey that we have.
      // FIXME: get this task from a pool. For now assume we can *always* get a new queue and therefore can create a new task::ImmediateSubmitQueue.
      vulkan::Queue queue;
      try
      {
        queue = m_submit_data.logical_device()->acquire_queue(m_submit_data.queue_request_key());
        Dout(dc::always, "Obtained queue: " << queue);
      }
      catch (vulkan::OutOfQueues_Exception const& error)
      {
        // No more queue.
        // FIXME: should be handled as part of the pool implementation.
        ASSERT(false);
      }
      auto immediate_submit_queue_task = statefultask::create<ImmediateSubmitQueue>(m_submit_data.logical_device(), queue COMMA_CWDEBUG_ONLY(mSMDebug));
      immediate_submit_queue_task->run(vulkan::Application::instance().medium_priority_queue());

      immediate_submit_queue_task->have_new_datum(std::move(m_submit_data));
      set_state(ImmediateSubmit_done);
      wait(commands_submitted);
      break;
    }
    case ImmediateSubmit_done:
      finish();
      break;
  }
}

} // namespace task
