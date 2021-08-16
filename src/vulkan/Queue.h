#pragma once

#include <vulkan/vulkan.hpp>
#include "QueueReply.h" // GPU_queue_family_handle

namespace vulkan {

class Queue
{
  GPU_queue_family_handle m_queue_family;
  vk::Queue m_queue;

 public:
  Queue() = default;
  Queue(GPU_queue_family_handle qfh, vk::Queue queue) : m_queue_family(qfh), m_queue(queue) { }

  vk::Queue index() const { return m_queue; }
  GPU_queue_family_handle queue_family() const { return m_queue_family; }
};

} // namespace vulkan
