#pragma once

#include "CommandBuffer.h"
#include "QueueRequestKey.h"
#include <functional>

namespace vulkan {

// This struct contains the data needed to start an ImmediateSubmit task.
struct ImmediateSubmitData
{
  static constexpr vk::CommandPoolCreateFlags::MaskType pool_type = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  using record_function_type = std::function<void(vulkan::CommandBufferWriteAccessType<pool_type>)>;

  vulkan::QueueRequestKey queue_request_key;    // Key that uniquely maps to a queue (request/reply) to use.
  record_function_type record_function;         // Callback function that will record the command buffer.
};

} // namespace vulkan
