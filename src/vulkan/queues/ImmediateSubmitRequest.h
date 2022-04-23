#pragma once

#include "CommandBuffer.h"
#include "QueueRequestKey.h"
#include <functional>

namespace vulkan {

// This struct contains the data needed to start an ImmediateSubmit task.
class ImmediateSubmitRequest
{
 public:
  static constexpr vk::CommandPoolCreateFlags::MaskType pool_type = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  using command_buffer_wat = vulkan::CommandBufferWriteAccessType<pool_type>;   // Wrapper around a vk::CommandBuffer.
  using record_function_type = std::function<void(command_buffer_wat)>;

 private:
  vulkan::LogicalDevice const* m_logical_device;
  vulkan::QueueRequestKey m_queue_request_key;  // Key that uniquely maps to a queue (request/reply) to use.
  record_function_type m_record_function;       // Callback function that will record the command buffer.

 public:
  ImmediateSubmitRequest() = default;
  ImmediateSubmitRequest(vulkan::LogicalDevice const* logical_device, vulkan::QueueRequestKey queue_request_key, record_function_type&& record_function) :
    m_logical_device(logical_device), m_queue_request_key(queue_request_key), m_record_function(std::move(record_function)) { }

  void set_logical_device(vulkan::LogicalDevice const* logical_device) { m_logical_device = logical_device; }
  void set_queue_request_key(vulkan::QueueRequestKey queue_request_key) { m_queue_request_key = queue_request_key; }
  void set_record_function(record_function_type&& record_function) { m_record_function = std::move(record_function); }

  vulkan::LogicalDevice const* logical_device() const
  {
    return m_logical_device;
  }

  vulkan::QueueRequestKey const& queue_request_key() const
  {
    return m_queue_request_key;
  }

  void record_commands(command_buffer_wat command_buffer_w) const
  {
    m_record_function(command_buffer_w);
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
