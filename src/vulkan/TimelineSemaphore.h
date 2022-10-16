#ifndef VULKAN_TIMELINE_SEMAPHORE_H
#define VULKAN_TIMELINE_SEMAPHORE_H

#include "statefultask/AIStatefulTask.h"
#include <vulkan/vulkan.hpp>
#include "debug.h"

namespace vulkan {

// Forward declaration.
class LogicalDevice;
class Ambifix;

class TimelineSemaphore
{
 private:
  LogicalDevice const* m_logical_device;        // The device associated with this semaphore.
  vk::UniqueSemaphore m_semaphore;
  uint64_t m_signal_value;                      // The next value to be used for a submit.

 public:
  // Create a timeline semaphore.
  inline TimelineSemaphore(LogicalDevice const* logical_device, uint64_t initial_value COMMA_CWDEBUG_ONLY(Ambifix const& ambifix));

  void signal(uint64_t value);
  bool wait_for(uint64_t value, uint64_t timeout_ns = uint64_t{0} - 1);

  // Add a poll for this timeline semaphore for the last signal value.
  inline void add_poll(AIStatefulTask* task, AIStatefulTask::condition_type condition) const;

  // Used to clean up: this only needs to be called if the TimelineSemaphore is destroyed before receiving a signal.
  inline void remove_poll() const;

  // Get the current value of the timeline semaphore.
  [[gnu::always_inline]] inline uint64_t get_counter_value() const;

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

#endif // VULKAN_TIMELINE_SEMAPHORE_H

#ifndef VULKAN_LOGICAL_DEVICE_H
#include "LogicalDevice.h"
#endif

#ifndef VULKAN_TIMELINE_SEMAPHORE_H_definitions
#define VULKAN_TIMELINE_SEMAPHORE_H_definitions

namespace vulkan {

// inline
TimelineSemaphore::TimelineSemaphore(LogicalDevice const* logical_device, uint64_t initial_value COMMA_CWDEBUG_ONLY(Ambifix const& ambifix)) :
  m_logical_device(logical_device),
  m_semaphore(logical_device->create_timeline_semaphore(initial_value COMMA_CWDEBUG_ONLY(".m_semaphore" + ambifix))),
  m_signal_value(initial_value) { }

// Add a poll for this timeline semaphore for the last signal value.
void TimelineSemaphore::add_poll(AIStatefulTask* task, AIStatefulTask::condition_type condition) const
{
  m_logical_device->add_timeline_semaphore_poll(this, m_signal_value, task, condition);
}

void TimelineSemaphore::remove_poll() const
{
  m_logical_device->remove_timeline_semaphore_poll(this);
}

uint64_t TimelineSemaphore::get_counter_value() const
{
  return m_logical_device->get_semaphore_counter_value(*m_semaphore);
}

} // namespace vulkan

#endif // VULKAN_TIMELINE_SEMAPHORE_H_definitions
