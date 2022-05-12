#include "sys.h"
#include "TimelineSemaphore.h"

namespace vulkan {

void TimelineSemaphore::signal(uint64_t value)
{
  vk::SemaphoreSignalInfo semaphore_signal_info{
    .semaphore = *m_semaphore,
    .value = value
  };
  m_logical_device->signal_timeline_semaphore(semaphore_signal_info);
}

bool TimelineSemaphore::wait_for(uint64_t value, uint64_t timeout_ns)
{
  vk::SemaphoreWaitInfo semaphore_wait_info{
    .flags = vk::SemaphoreWaitFlagBits::eAny,
    .semaphoreCount = 1,
    .pSemaphores = &*m_semaphore,
    .pValues = &value
  };
  vk::Result res = m_logical_device->wait_semaphores(semaphore_wait_info, timeout_ns);
  return res == vk::Result::eSuccess; // Otherwise eTimeout.
}

} // namespace vulkan
