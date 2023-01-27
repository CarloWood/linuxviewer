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

class VertexBuffers;

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
//   command_buffer.begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
//   ...
//   command_buffer.end();
//
// Finally, a vulkan::CommandBuffer* can be converted to the underlying
// vk::CommandBuffer, so that it can be submitted using:
//
//   ...
//   .commandBufferCount = command_buffers.size(),
//   .pCommandBuffers = command_buffers.data()
//   ...
//
class CommandBuffer : public vk::CommandBuffer
{
 private:
  // Needed to allocate new buffers.
  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  friend class vulkan::CommandPool;

 public:
  // Default constructor.
  CommandBuffer() = default;

  [[gnu::always_inline]] inline void bindVertexBuffers(VertexBuffers const& vertex_buffers);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

// Among others see CommandPool<pool_type>::allocate_buffers and CommandPool<pool_type>::free_buffers
// and of course when used as array (i.e. for .pCommandBuffers).
static_assert(sizeof(CommandBuffer) == sizeof(vk::CommandBuffer), "handle::CommandBuffer must be a thin wrapper around a vk::CommandBuffer.");

} // namespace handle
} // namespace vulkan

#endif // COMMAND_BUFFER_H
