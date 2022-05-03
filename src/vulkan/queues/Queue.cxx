#include "sys.h"
#include "TimelineSemaphore.h"
#include "utils/AIAlert.h"

#ifdef CWDEBUG
#include "Queue.h"
#include "debug_ostream_operators.h"
#endif

namespace vulkan {

#ifdef CWDEBUG
void Queue::print_on(std::ostream& os) const
{
  os << '{';
  os << "m_queue_family:" << m_queue_family << ", ";
  os << "m_vh_queue:" << m_vh_queue;
  os << '}';
}
#endif

void Queue::submit(vk::CommandBuffer const* vh_command_buffer_ptr, TimelineSemaphore& timeline_semaphore)
{
  vk::TimelineSemaphoreSubmitInfo timeline_semaphore_info{
    .signalSemaphoreValueCount = 1,
    .pSignalSemaphoreValues = timeline_semaphore.get_next_value_ptr()   // Returns a reference; is not thread-safe.
                                                                        // No other thread may call get_next_value() until after submit() below returned.
  };

  vk::SubmitInfo submit_info{
    .pNext = &timeline_semaphore_info,
    .waitSemaphoreCount = 0,
    .pWaitSemaphores = nullptr,
    .pWaitDstStageMask = nullptr,
    .commandBufferCount = 1,
    .pCommandBuffers = vh_command_buffer_ptr,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = timeline_semaphore.vh_semaphore_ptr()
  };

  vk::Result res = m_vh_queue.submit(1, &submit_info, nullptr);
  if (res != vk::Result::eSuccess)
    THROW_ALERTC(res, "Queue::submit");
}

} // namespace vulkan
