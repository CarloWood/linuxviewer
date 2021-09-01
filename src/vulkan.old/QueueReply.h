#pragma once

#include "utils/Vector.h"
#include <cstdint>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

// Queue family handles (uint32_t) are the index into the vector returned
// by vk::PhysicalDevice::getQueueFamilyProperties().
struct GPU_queue_family_handle_category {};
using GPU_queue_family_handle = utils::VectorIndex<GPU_queue_family_handle_category>;

// Each QueueRequest is paired with one QueueReply.
class QueueReply
{
 private:
  GPU_queue_family_handle m_queue_family_handle;        // The queue family that should be used.
  uint32_t m_number_of_queues;                          // The actual number of queues that should be allocated.

 public:
  QueueReply() = default;

  QueueReply(GPU_queue_family_handle queue_family_handle, uint32_t number_of_queues) :
    m_queue_family_handle(queue_family_handle), m_number_of_queues(number_of_queues) { }

  QueueReply& operator=(QueueReply const& rhs)
  {
    m_queue_family_handle = rhs.m_queue_family_handle;
    m_number_of_queues = rhs.m_number_of_queues;
    return *this;
  }

  GPU_queue_family_handle get_queue_family_handle() const
  {
    return m_queue_family_handle;
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
