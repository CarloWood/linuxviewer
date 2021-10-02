#pragma once

#include "QueueFamilyProperties.h"
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

// Each QueueRequest is paired with one QueueReply.
class QueueReply
{
 private:
  QueueFamilyPropertiesIndex m_queue_family;    // The queue family that should be used.
  uint32_t m_number_of_queues;                  // The actual number of queues that should be allocated.

 public:
  QueueReply() = default;

  QueueReply(QueueFamilyPropertiesIndex queue_family, uint32_t number_of_queues) :
    m_queue_family(queue_family), m_number_of_queues(number_of_queues) { }

  QueueReply& operator=(QueueReply const& rhs)
  {
    m_queue_family = rhs.m_queue_family;
    m_number_of_queues = rhs.m_number_of_queues;
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

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
