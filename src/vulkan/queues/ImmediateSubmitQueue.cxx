#include "sys.h"
#include "ImmediateSubmitQueue.h"
#include "CommandBufferFactory.h"
#include "utils/AIAlert.h"

namespace task {

ImmediateSubmitQueue::ImmediateSubmitQueue(
    vulkan::LogicalDevice const* logical_device,
    vulkan::Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug)) :
  direct_base_type(CWDEBUG_ONLY(debug)),
  m_command_buffer_pool(8, m_deque_allocator, logical_device, queue.queue_family()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_command_buffer_pool.m_factory"))),
  m_queue(queue),
  m_semaphore(logical_device, 0
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_timeline_semaphore")))
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
    {
      // Reserve up to 64 elements for reading.
      int n = 64;
      container_type::const_iterator submit_request = front_n(n);
      if (n > 0)
      {
        // Acquire n command buffers from the command buffer pool.
        std::vector<vulkan::handle::CommandBuffer> command_buffers(n);
        // Attempt to acquire n buffers - this might fail.
        size_t acquired = m_command_buffer_pool.acquire(command_buffers);
        if (AI_LIKELY(acquired > 0))
        {
          // As this task owns the deque and is essentially single threaded,
          // we can now simply iterate over the elements returned by front_n
          // without having the deque locked: producer threads can add new
          // elements in the meantime without invalidating submit_request.
          for (int count = 0; count < acquired; ++count, ++submit_request)
          {
            Dout(dc::vulkan, "ImmediateSubmitQueue_need_action: received submit_request: " << *submit_request << " [" << this << "]");

            // Get accessor for command buffer 'count' (a no-op in Release mode).
            auto command_buffer_w = command_buffers[count](m_command_buffer_pool.factory().command_pool());
            // Record the command buffer.
            submit_request->record_commands(command_buffer_w);
            // Submit recorded commands.
            m_queue.submit(command_buffer_w, m_semaphore);
            // Inform producer task that the commands were issued.
            submit_request->issued(m_semaphore.signal_value());

            // Prevent submit_request from being moved past the last allocated command buffer.
            // This is required for the call to pop_front_n below.
            if (count == acquired - 1)
              break;
          }
          // Release the used command buffers.
          m_command_buffer_pool.release(command_buffers);
          // Erase the acquired submit requests that were just processed.
          pop_front_n(submit_request);
        }
        // If less command buffers were returned then requested, then request for a callback once those command buffers are available again.
        // The re-use of `need_action` is kindof iffy, but because that is the ONLY signal that this task has and it is as general as
        // "do something" (action needs to be taken) it will work: this just assures this task will run again once more CAN be done.
        // It will also still run again when more submit requests are added; that then can result in multiple calls to the below 'subscribe'
        // function - so that must be able to deal with that.
        if (acquired < n)
          m_command_buffer_pool.subscribe(n - acquired, this, need_action);
      }
      if (producer_not_finished())
        break;
      set_state(ImmediateSubmitQueue_done);
      [[fallthrough]];
    }
    case ImmediateSubmitQueue_done:
      finish();
      break;
  }
}

void ImmediateSubmitQueue::terminate()
{
  // Calling set_producer_finished() instead would work too; but that would handle
  // all immediate submit requests that are still queued up and wait for the current
  // one to be finished. We do not need to do that.
  abort();
}

} // namespace task
