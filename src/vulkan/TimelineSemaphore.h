#pragma once

#include <vulkan/vulkan.hpp>
#include "LogicalDevice.h"

namespace vulkan {

class TimelineSemaphore
{
 private:
  LogicalDevice const* m_logical_device;        // The device associated with this semaphore.
  vk::UniqueSemaphore m_semaphore;
  uint64_t m_signal_value;                      // The next value to be used for a submit.

 public:
  // Create a timeline semaphore.
  TimelineSemaphore(LogicalDevice const* logical_device, uint64_t initial_value COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
    m_logical_device(logical_device),
    m_semaphore(logical_device->create_timeline_semaphore(initial_value COMMA_CWDEBUG_ONLY(ambifix(".m_semaphore")))),
    m_signal_value(initial_value) { }

  void signal(uint64_t value)
  {
    vk::SemaphoreSignalInfo semaphore_signal_info{
      .semaphore = *m_semaphore,
      .value = value
    };
    m_logical_device->signal_timeline_semaphore(semaphore_signal_info);
  }

  bool wait_for(uint64_t value, uint64_t timeout_ns = uint64_t{0} - 1)
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

  uint64_t get_counter_value() const
  {
    return m_logical_device->get_semaphore_counter_value(*m_semaphore);
  }

  // Returns a pointer to an uint64_t with the same life-time as this object.
  uint64_t const* get_next_value_ptr()
  {
    ++m_signal_value;
    return &m_signal_value;
  }

  // Return the value that was last returned by get_next_value_ptr.
  uint64_t signal_value() const
  {
    return m_signal_value;
  }

  vk::Semaphore const* vh_semaphore_ptr() const
  {
    return &*m_semaphore;       // operator* returns a reference.
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const
  {
    os << "{logical_device:" << m_logical_device << ", semaphore:" << *m_semaphore << ", signal_value:" << m_signal_value << "; counter = " << get_counter_value() << '}';
  }
#endif
};

} // namespace vulkan
