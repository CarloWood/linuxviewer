#include "sys.h"
#include "ImmediateSubmitQueue.h"
#include "CommandBufferFactory.h"
#include "utils/AIAlert.h"

namespace vulkan::task {

ImmediateSubmitQueue::ImmediateSubmitQueue(
    vulkan::LogicalDevice const* logical_device,
    Queue const& queue
    COMMA_CWDEBUG_ONLY(bool debug)) :
  direct_base_type(CWDEBUG_ONLY(debug)),
  m_command_buffer_pool(8, m_deque_allocator, logical_device, queue.queue_family()
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_command_buffer_pool.m_factory"))),
  m_queue(queue),
  m_semaphore(logical_device, 0
      COMMA_CWDEBUG_ONLY(debug_name_prefix("m_timeline_semaphore")))
{
  DoutEntering(dc::statefultask(mSMDebug), "ImmediateSubmitQueue(" << logical_device << ", " << queue << ") [" << this << "]");
}

ImmediateSubmitQueue::~ImmediateSubmitQueue()
{
  DoutEntering(dc::statefultask(mSMDebug), "~ImmediateSubmitQueue() [" << this << "]");
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

char const* ImmediateSubmitQueue::task_name_impl() const
{
  return "ImmediateSubmitQueue";
}

void ImmediateSubmitQueue::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case ImmediateSubmitQueue_need_action:
    {
      // The deque of the TaskToTaskDeque base class is appended to at the end by "producer tasks".
      // We only obtain the lock on this deque while accessing it through base class member functions,
      // so new elements can be appended at any moment, invalidating the `end()` iterator.
      // Therefore we can't use the `end()` iterator and use instead a count of elements that are
      // known to be in the deque (because it only grows; this task being the only consumer).
      //
      // Moreover, this deque is used for a dual purpose: to submit new immediate-submit requests,
      // all stored at the end, but also to keep track of already submitted requests, the command
      // buffer used for them and the signal value of the timeline semaphore used for that submit.
      // Those are all elements at the beginning of the deque.
      //
      //                  std::deque<ImmediateSubmitRequest>
      //                             .----------.
      //                             |          | ⎞ <-- begin() iff m_submitted > 0.
      //                             |          | ⎟- m_pending_requests = number of submitted, but not finished, command buffers.
      //                             |          | ⎟
      //  m_last_submitted       --> |          | ⎠
      //   (only valid if            +----------+
      //    m_pending_requests > 0)  |          | ⎞⎞  <-- first_submit_request (= begin() iff m_submitted == 0) (only valid if n > 0).
      //                             |          | ⎟⎟- n = number of existing new submit requests at the time of the call to front_n(n) (with a max. of 64).
      //                             |          | ⎟-- acquired = number of command buffers acquired (might be less than n).
      //                             |          | ⎠⎟
      //                             |          |  ⎠
      //                             .          .
      //                             .          . <-- possibly already appended new submit requests (there is no `end()`).
      //
      //
      // Reserve up to 64 elements for reading.
      int n = m_pending_requests + 64;
      // front_n sets n to the size of the deque (if that is less than the n that is passed).
      container_type::const_iterator const first_pending_request = front_n(n);  // Returns begin(); points to ImmediateSubmitRequest's.
      // We know there are this many elements in the deque; so it should be impossible for n to be less.
      ASSERT(n >= m_pending_requests);
      // Set n to the number of existing new submit requests.
      n -= m_pending_requests;
      // If there are pending requests then the first newly submitted request is at m_last_submitted + 1, otherwise it is at begin().
      container_type::const_iterator const first_submit_request = (m_pending_requests > 0) ? m_last_submitted + 1 : first_pending_request;
      if (m_pending_requests > 0)
      {
        Dout(dc::vulkan, "Considering " << m_pending_requests << " pending requests.");
        std::vector<handle::CommandBuffer> command_buffers(m_pending_requests);
        container_type::const_iterator pending_request = first_pending_request;
        uint64_t counter_value = m_semaphore.get_counter_value();
        int processed = 0;
        for (;;)
        {
          if (counter_value < pending_request->signal_value())
          {
            // This one should not be processed anymore. Hence, the previous one -if any- was the last one that was processed.
            if (processed > 0)
              --pending_request;        // Must be equal to the last processed request for the call to pop_front_n below.
            break;
          }
          command_buffers[processed++] = pending_request->command_buffer();
          pending_request->finished();
          // Do not increment pending_request past the last one processed.
          if (processed == m_pending_requests)
            break;
          ++pending_request;
        }
        Dout(dc::vulkan, "Processed " << processed << " pending requests.");
        if (processed > 0)
        {
          // Release the command buffers of the pending requests that were signaled.
          m_command_buffer_pool.release(command_buffers.data(), processed);
          // Erase the pending requests that were just processed.
          pop_front_n(pending_request);         // If this invalidates m_last_submitted
          m_pending_requests -= processed;      // then this will become zero.
        }
      }
      if (n > 0)
      {
        // Acquire n command buffers from the command buffer pool.
        std::vector<handle::CommandBuffer> command_buffers(n);
        // Attempt to acquire n buffers - this might fail.
        Debug(m_command_buffer_pool.factory().set_ambifix({"ImmediateSubmitQueue_need_action::command_buffers", as_postfix(this)}));
        size_t const acquired = m_command_buffer_pool.acquire(command_buffers);
        if (AI_LIKELY(acquired > 0))
        {
          // As this task owns the deque and is essentially single threaded, we can
          // now simply iterate over the elements, starting with first_submit_request
          // without having the deque locked: producer threads can add new elements
          // in the meantime without invalidating submit_request.
          container_type::const_iterator submit_request = first_submit_request;
          int count = 0;
          for (;;)
          {
            Dout(dc::vulkan, "ImmediateSubmitQueue_need_action: received submit_request: " << *submit_request << " [" << this << "]");

            // Record the command buffer.
            submit_request->record_commands(command_buffers[count]);
            // Store pending request data. Because this is the only thread/task that uses m_semaphore; it is safe to
            // add 1 to the value returned by signal_value() and assume that will be the value used by the submit below.
            submit_request->set_command_buffer_and_signal_value(command_buffers[count], m_semaphore.signal_value() + 1);
            // Prevent submit_request from being moved past the last recorded command buffer.
            if (++count == acquired)
              break;
            ++submit_request;
          }
          m_last_submitted = submit_request;
          m_pending_requests += acquired;

          // Submit recorded commands.
          m_queue.submit(command_buffers.data(), acquired, m_semaphore);

          // Wake me up when you're done.
          m_semaphore.add_poll(this, need_action);
        }
        // If less command buffers were returned than requested, then request for a callback once those command buffers are available again.
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
      Dout(dc::statefultask(mSMDebug), "Falling through to ImmediateSubmitQueue_done.");
      [[fallthrough]];
    }
    case ImmediateSubmitQueue_done:
      finish();
      break;
  }
}

void ImmediateSubmitQueue::abort_impl()
{
  m_semaphore.remove_poll();
  flush_new_data([](ImmediateSubmitRequest&& submit_request){
    submit_request.abort();
  });
}

void ImmediateSubmitQueue::terminate()
{
  // Calling set_producer_finished() instead would work too; but that would handle
  // all immediate submit requests that are still queued up and wait for the current
  // one to be finished. We do not need to do that.
  abort();
}

} // namespace vulkan::task
