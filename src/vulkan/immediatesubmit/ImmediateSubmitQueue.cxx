#include "sys.h"
#include "ImmediateSubmitQueue.h"

namespace task {

ImmediateSubmitQueue::ImmediateSubmitQueue(
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(vulkan::AmbifixOwner const& command_pool_debug_name, bool debug)) :
  direct_base_type(CWDEBUG_ONLY(debug)),
  m_command_pool(logical_device, queue.queue_family() COMMA_CWDEBUG_ONLY(command_pool_debug_name))
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
    AI_CASE_RETURN(ImmediateSubmitQueue_start);
    AI_CASE_RETURN(ImmediateSubmitQueue_done);
  }
  AI_NEVER_REACHED
}

void ImmediateSubmitQueue::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmitQueue_start:
      break;
    case ImmediateSubmitQueue_done:
      break;
  }
}

} // namespace task
