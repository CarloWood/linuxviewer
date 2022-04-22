#include "sys.h"
#include "ImmediateSubmitQueue.h"

namespace task {

ImmediateSubmitQueue::ImmediateSubmitQueue(
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug)) :
  direct_base_type(CWDEBUG_ONLY(debug)),
  m_command_pool(logical_device, queue.queue_family()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_command_pool")))
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmitQueue::ImmediateSubmitQueue(" << logical_device << ", " << queue << ") [" << this << "]");
}

ImmediateSubmitQueue::~ImmediateSubmitQueue()
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmitQueue::~ImmediateSubmitQueue() [" << this << "]");
}

char const* ImmediateSubmitQueue::state_str_impl(state_type run_state) const
{
  switch (run_state)
  {
    AI_CASE_RETURN(ImmediateSubmitQueue_need_action);
    AI_CASE_RETURN(ImmediateSubmitQueue_done);
  }
  AI_NEVER_REACHED
}

void ImmediateSubmitQueue::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmitQueue_need_action:
      flush_new_data([this](Datum&& datum){
        Dout(dc::always, "ImmediateSubmitQueue_need_action: received " << datum);
      });
      if (producer_not_finished())
        break;
      set_state(ImmediateSubmitQueue_done);
      [[fallthrough]];
    case ImmediateSubmitQueue_done:
      break;
  }
}

} // namespace task
