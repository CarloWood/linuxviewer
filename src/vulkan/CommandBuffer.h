#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <vulkan/vulkan.hpp>
#include "threadsafe/aithreadsafe.h"
#ifdef CWDEBUG
#include <cstdint>      // uint64_t
#endif

namespace vulkan {

#ifdef CWDEBUG
namespace details {
using UniquePoolID = uint64_t;
} // namespace details
#endif

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class UnlockedCommandPool;

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class CommandBufferWriteAccessType;

namespace handle {

// class CommandBuffer.
//
// Usage:
//
// Define command pools anywhere (ie, on the stack when it is a temporary command pool, or as member variable of some class).
// The VkCommandPoolCreateFlagBits bits are part of the type:
//
//   using command_pool_type = CommandPool<VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT>;
//   command_pool_type command_pool;
//
// Define command buffer objects anywhere:
//
//   CommandBuffer command_buffer;
//   std::array<CommandBuffer, 10> command_buffers_array;
//   std::vector<CommandBuffer> command_buffers_vector;
//
// In order to allocate command buffers, the pool must be locked:
//
//   command_pool_type::wat command_pool_w(command_pool);
//
// This should be an automatic variable.
// Then, while it exists (you can pass it to functions as argument though), you can
// allocate command buffer(s).
//
// In debug mode this requires to pass a debug name that will be used for the command
// allocated command buffers. See explanation of `AmbifixOwner` elsewhere.
//
//   command_buffer = command_pool_w->allocate_buffer(
//       CWDEBUG_ONLY(debug_name_prefix("command_buffer"))
//       );
//
//   command_pool_w->allocate_buffers(command_buffers_array
//       COMMA_CWDEBUG_ONLY(debug_name_prefix("command_buffers_array"))
//       );
//
//   command_buffers_vector.resize(size);
//   command_pool_w->allocate_buffers(command_buffers_vector
//       COMMA_CWDEBUG_ONLY(debug_name_prefix("command_buffers_vector"))
//       );
//
// After allocation the command pool can be unlocked again by
// destructing command_pool_w (aka, leaving scope).
//
// In order to use a command buffer, the pool must be locked again
// and then an object must be created on the stack to access the command buffer:
//
//   command_pool_type::wat command_pool_w(command_pool);
//   auto command_buffer_w = command_buffer(command_pool_w);
//
// Now the command buffer can be accessed using operator-> which returns a vk::CommandBuffer*.
// For example,
//
//   command_buffer_w->begin({ .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit });
//   ...
//   command_buffer_w->end();
//
// If only one command buffer needs to be accessed, the first two lines
// may NOT be abbreviated as follows:
//
//   auto command_buffer_w = command_buffer(command_pool_type::wat(command_pool));
//
// Otherwise the pool will be unlocked while using command_buffer_w.
//
// In order to avoid this error, CommandBuffer::operator()(CommandPool::wat&&)
// exists and is deleted.
//
// Finally, command_buffer_w will implicitely convert to the pointer of
// the underlaying vk::CommandBuffer, so that it can be submitted using:
//
//   ...
//   .commandBufferCount = 1,
//   .pCommandBuffers = tmp_command_buffer_w
//   ...
//
class CommandBuffer
{
 private:
  vk::CommandBuffer m_handle;           // The underlaying vulkan handle.

#ifdef CWDEBUG
  details::UniquePoolID m_pool_id;      // For debug purposes; a unique ID of the command pool that this buffer was allocated from.

  // Constructor to initialize the pool ID.
  CommandBuffer(details::UniquePoolID pool_id) : m_pool_id(pool_id) { }
#endif

  // Needed to allocate new buffers.
  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  friend class vulkan::UnlockedCommandPool;

  // Needs access to m_handle.
  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  friend class vulkan::CommandBufferWriteAccessType;

 public:
  // Default constructor.
  CommandBuffer() = default;

  // CommandBufferWriteAccessType factory.
  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  CommandBufferWriteAccessType<pool_type> operator()(
      // This type is just CommandPool::wat const&, but if we use that it can't infer the template argument `pool_type`.
      aithreadsafe::Access<aithreadsafe::Wrapper<UnlockedCommandPool<pool_type>, aithreadsafe::policy::Primitive<std::mutex>>> const& command_pool_w
      ) const;

  template<vk::CommandPoolCreateFlags::MaskType pool_type>
  CommandBufferWriteAccessType<pool_type> operator()( // Do not accept a temporary! The life time of command_pool_w must exceed that of the returned object.
      aithreadsafe::Access<aithreadsafe::Wrapper<UnlockedCommandPool<pool_type>, aithreadsafe::policy::Primitive<std::mutex>>>&& command_pool_w
      ) const = delete;
};

} // namespace handle
} // namespace vulkan

// See https://stackoverflow.com/questions/55751848/how-can-i-handle-circular-include-calls-in-a-template-class-header/69873866#69873866
#include "CommandPool.h"
#endif // COMMAND_BUFFER_H

#ifndef COMMAND_BUFFER_DEFINITION_H
#define COMMAND_BUFFER_DEFINITION_H

namespace vulkan {

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class CommandBufferWriteAccessType
{
  vk::CommandBuffer m_handle;           // A copy of the CommandBuffer::m_handle that we give access to.

  // Construct a CommandBufferWriteAccessType from a command pool and a command buffer.
  CommandBufferWriteAccessType(typename CommandPool<pool_type>::wat const& CWDEBUG_ONLY(pool_w), handle::CommandBuffer const& command_buffer) :
    m_handle(command_buffer.m_handle)
  {
#ifdef CWDEBUG
    ASSERT(pool_w->id() == command_buffer.m_pool_id);
#endif
  }

  // Factory for CommandBufferWriteAccessType objects (the constructor is private).
  // g++ doesn't compile when omitting the template<> and using pool_type directly (clang++ works).
  template<vk::CommandPoolCreateFlags::MaskType pool_type2>
  friend CommandBufferWriteAccessType<pool_type2> handle::CommandBuffer::operator()(
    aithreadsafe::Access<aithreadsafe::Wrapper<UnlockedCommandPool<pool_type2>, aithreadsafe::policy::Primitive<std::mutex>>> const& command_pool_w) const;

 public:
  // Accessor for the underlaying command buffer.
  typename vk::CommandBuffer* operator->()
  {
    return &m_handle;
  }

  // Used when submitting.
  operator vk::CommandBuffer*()
  {
    return &m_handle;
  }
};

namespace handle {

template<vk::CommandPoolCreateFlags::MaskType pool_type>
CommandBufferWriteAccessType<pool_type> CommandBuffer::operator()(
    aithreadsafe::Access<aithreadsafe::Wrapper<UnlockedCommandPool<pool_type>, aithreadsafe::policy::Primitive<std::mutex>>> const& command_pool_w) const
{
  return {command_pool_w, *this};
}

} // namespace handle

} // namespace vulkan

#endif // COMMAND_BUFFER_DEFINITION_H
