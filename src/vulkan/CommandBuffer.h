#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "threadsafe/aithreadsafe.h"
#ifdef CWDEBUG
#include <cstdint>      // uint64_t
#include <iosfwd>
#endif

namespace vulkan {

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class CommandPool;

namespace handle {

// class CommandBuffer.
//
// Usage:
//
// Define command pools as part of a task.
// The VkCommandPoolCreateFlagBits bits are part of the type:
//
//   using command_pool_type = vulkan::CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT>;
//   command_pool_type m_command_pool;
//
// Define command buffer objects as part of the same task, or as local variables
// in functions executed by the task that owns the pool.
//
//   vulkan::CommandBuffer m_command_buffer;
//   std::array<vulkan::CommandBuffer, 10> m_command_buffers_array;
//   std::vector<vulkan::CommandBuffer> command_buffers_vector;
//
// Allocating command buffers may only be done by the task that owns the command pool.
// In debug mode this requires to pass a debug name that will be used for the
// allocated command buffers. See explanation of `Ambifix` elsewhere.
//
//   m_command_buffer = m_command_pool.allocate_buffer(
//       CWDEBUG_ONLY(debug_name_prefix("m_command_buffer"))
//       );
//
//   m_command_pool.allocate_buffers(m_command_buffers_array
//       COMMA_CWDEBUG_ONLY(debug_name_prefix("m_command_buffers_array"))
//       );
//
//   command_buffers_vector.resize(size);
//   m_command_pool.allocate_buffers(command_buffers_vector
//       COMMA_CWDEBUG_ONLY(debug_name_prefix("function()::command_buffers_vector"))
//       );
//
// The command buffer may only be used while no other thread uses the command pool,
// or other command buffers allocated from the same pool. For example, by the task
// that owns the pool, or by a task that was spawned by that task while it is waiting.
//
// Now the command buffer can be accessed using operator-> which returns a vk::CommandBuffer*.
// For example,
//
//   command_buffer->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
//   ...
//   command_buffer->end();
//
// Finally, a vulkan::CommandBuffer* can be converted to the underlaying
// vk::CommandBuffer, so that it can be submitted using:
//
//   ...
//   .commandBufferCount = count,
//   .pCommandBuffers = command_buffers->get_array()
//   ...
//
class CommandBuffer
{
 private:
  vk::CommandBuffer m_vh_command_buffer;           // The underlaying vulkan handle.

  // Needed to allocate new buffers.
  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  friend class vulkan::CommandPool;

 public:
  // Default constructor.
  CommandBuffer() = default;

  vk::CommandBuffer* get_array() { return &m_vh_command_buffer; }
  vk::CommandBuffer const* get_array() const { return &m_vh_command_buffer; }

  vk::CommandBuffer* operator->() { return &m_vh_command_buffer; }
  vk::CommandBuffer const* operator->() const { return &m_vh_command_buffer; }

  operator vk::CommandBuffer() const { return m_vh_command_buffer; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace handle
} // namespace vulkan

#endif // COMMAND_BUFFER_H
