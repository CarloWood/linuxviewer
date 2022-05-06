#pragma once

#include "LogicalDevice.h"
#include "CommandBuffer.h"
#include "QueueRequestKey.h"
#include <functional>

namespace task {
class ImmediateSubmit;
} // namespace task

namespace vulkan {

// This struct contains the data needed to start an ImmediateSubmit task.
class ImmediateSubmitRequest
{
 public:
  static constexpr vk::CommandPoolCreateFlags::MaskType pool_type = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  using record_function_type = std::function<void(handle::CommandBuffer)>;

 private:
  task::ImmediateSubmit* m_immediate_submit;            // The ImmediateSubmit ask that issued this request.
  LogicalDevice const* m_logical_device;                // The logical device to use.
  vulkan::QueueRequestKey m_queue_request_key;          // Key that uniquely maps to a queue (request/reply) to use.
  record_function_type m_record_function;               // Callback function that will record the command buffer.

 public:
  ImmediateSubmitRequest() = default;
  ImmediateSubmitRequest(ImmediateSubmitRequest&&) = default;
  ImmediateSubmitRequest(vulkan::LogicalDevice const* logical_device, task::ImmediateSubmit* immediate_submit) :
    m_immediate_submit(immediate_submit), m_logical_device(logical_device), m_queue_request_key({QueueFlagBits::eTransfer, logical_device->transfer_request_cookie()}) { }
#if 0 // Are these ever used?
  ImmediateSubmitRequest(vulkan::LogicalDevice const* logical_device, vulkan::QueueRequestKey queue_request_key) :
    m_immediate_submit(nullptr), m_logical_device(logical_device), m_queue_request_key(queue_request_key) { }
  ImmediateSubmitRequest(vulkan::LogicalDevice const* logical_device, vulkan::QueueRequestKey queue_request_key, record_function_type&& record_function) :
    m_immediate_submit(nullptr), m_logical_device(logical_device), m_queue_request_key(queue_request_key), m_record_function(std::move(record_function)) { }
#endif
  ImmediateSubmitRequest& operator=(ImmediateSubmitRequest&& orig)
  {
    m_immediate_submit = orig.m_immediate_submit;
    m_logical_device = orig.m_logical_device;
    m_queue_request_key = orig.m_queue_request_key;
    m_record_function = std::move(orig.m_record_function);
    return *this;
  }

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

  void record_commands(handle::CommandBuffer command_buffer) const
  {
    m_record_function(command_buffer);
  }

  void issued(uint64_t signal_value) const;
  void finished() const;

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
