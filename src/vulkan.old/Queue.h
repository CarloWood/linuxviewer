#pragma once

#include <vulkan/vulkan.hpp>
#include "QueueReply.h" // GPU_queue_family_handle

#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan {

class Queue
{
  GPU_queue_family_handle m_queue_family;
  vk::Queue m_vh_queue;

 public:
  Queue() = default;
  Queue(GPU_queue_family_handle qfh, vk::Queue vh_queue) : m_queue_family(qfh), m_vh_queue(vh_queue) { }

  vk::Queue vh_queue() const { return m_vh_queue; }
  GPU_queue_family_handle queue_family() const { return m_queue_family; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
