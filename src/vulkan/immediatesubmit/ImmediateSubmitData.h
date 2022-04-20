#pragma once

#include "CommandBuffer.h"
#include "QueueRequestKey.h"
#include <functional>

namespace vulkan {

// This struct contains the data needed to start an ImmediateSubmit task.
class ImmediateSubmitData
{
 public:
  static constexpr vk::CommandPoolCreateFlags::MaskType pool_type = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  using record_function_type = std::function<void(vulkan::CommandBufferWriteAccessType<pool_type>)>;

 private:
  vulkan::QueueRequestKey m_queue_request_key;  // Key that uniquely maps to a queue (request/reply) to use.
  record_function_type m_record_function;       // Callback function that will record the command buffer.

 public:
  ImmediateSubmitData(vulkan::QueueRequestKey queue_request_key, std::function<void(vulkan::CommandBufferWriteAccessType<pool_type>)>&& record_function) :
    m_queue_request_key(queue_request_key), m_record_function(std::move(record_function)) { }
};

} // namespace vulkan
