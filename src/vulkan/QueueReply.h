#pragma once

#include "QueueFamilyProperties.h"
#include <cstdint>
#include <atomic>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

// Each QueueRequest is paired with one QueueReply.
class QueueReply
{
 private:
  QueueFamilyPropertiesIndex m_queue_family;    // The queue family that should be used.
  uint32_t m_number_of_queues;                  // The number of queues that can be allocated; this might be less than the max_number_of_queues
                                                // of the corresponding QueueRequest, or even larger if this reply was combined with another (but
                                                // not more than the max_number_of_queues of the QueueRequest it was combined with).
  QueueFlags m_requested_queue_flags;           // The requested queue flags (copy of corresponding QueueRequest).
  uint32_t m_start_index = {};                  // The index of the first queue of this Reply in it respective queue family.
  QueueRequestIndex m_combined_with;            // Set when this is a duplicate of the Reply that it was combined with.
  std::atomic<uint32_t> m_acquired;             // The number of queues of this pool that were already acquired (with LogicalDevice::acquire_queue).
                                                // Hence 0 <= m_acquired <= m_number_of_queues.

 public:
  QueueReply() = default; // QueueRequest

  QueueReply(QueueFamilyPropertiesIndex queue_family, uint32_t number_of_queues, QueueFlags requested_queue_flags, QueueRequestIndex combined_with = {}) :
    m_queue_family(queue_family), m_number_of_queues(number_of_queues), m_requested_queue_flags(requested_queue_flags), m_combined_with(combined_with) { }

  QueueReply& operator=(QueueReply const& rhs)
  {
    m_queue_family = rhs.m_queue_family;
    m_number_of_queues = rhs.m_number_of_queues;
    m_requested_queue_flags = rhs.m_requested_queue_flags;
    m_combined_with= rhs.m_combined_with;
    return *this;
  }

  QueueFamilyPropertiesIndex get_queue_family() const
  {
    return m_queue_family;
  }

  uint32_t number_of_queues() const
  {
    return m_number_of_queues;
  }

  QueueFlags requested_queue_flags() const
  {
    return m_requested_queue_flags;
  }

  QueueRequestIndex combined_with() const
  {
    return m_combined_with;
  }

  void set_start_index(uint32_t start_index)
  {
    m_start_index = start_index;
  }

  // Should ONLY be called by LogicalDevice::acquire_queue.
  // Returns the next queue index, if any is still free.
  int acquire_queue()
  {
    int next_queue_index = m_acquired.fetch_add(1);
    if (next_queue_index >= m_number_of_queues)
    {
      m_acquired -= 1;
      return -1;
    }
    return m_start_index + next_queue_index;
  }

  void release_queue()
  {
    int old_acquired = m_acquired.fetch_sub(1);
    // Calling release_queue more often than acquire_queue!
    ASSERT(old_acquired >= 0);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
