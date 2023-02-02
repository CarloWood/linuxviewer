#include "sys.h"
#include "SemaphoreWatcher.h"

namespace vulkan::task {

char const* AsyncSemaphoreWatcher::condition_str_impl(condition_type condition) const
{
  switch (condition)
  {
    AI_CASE_RETURN(poll_timer);
  }
  return SemaphoreWatcher<AsyncTask>::condition_str_impl(condition);
}

void AsyncSemaphoreWatcher::multiplex_impl(state_type run_state)
{
  switch (run_state)
  {
    case SemaphoreWatcher_poll:
      m_poll_rate_limiter.start(m_poll_rate_interval);
      if (poll())
      {
        yield();
        wait(poll_timer);
      }
      else
      {
        if (!m_poll_rate_limiter.stop())
        {
          // We could not stop the timer from firing. Perhaps because it already
          // fired, or because it is already calling expire(). Wait until it
          // returned from expire().
          m_poll_rate_limiter.wait_for_possible_expire_to_finish();
          // Now we are sure that signal(poll_timer) was already called.
          // To nullify that, call wait(poll_timer) here.
          wait(poll_timer);
        }
        wait(have_semaphores);
      }
      break;
    case SemaphoreWatcher_done:
      finish();
      break;
  }
}

} // namespace vulkan::task
