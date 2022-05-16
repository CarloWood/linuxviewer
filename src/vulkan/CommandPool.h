#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H

#include "LogicalDevice.h"
#include "queues/QueueFamilyProperties.h"
#include "threadsafe/aithreadsafe.h"
#include <vector>

namespace vulkan {
namespace details {

template<uint32_t flags>
struct is_transient_or_reset_only
{
  constexpr static bool value = (flags & (VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) == flags;
};

template<uint32_t flags>
inline constexpr bool is_transient_or_reset_only_v = is_transient_or_reset_only<flags>::value;

} // namespace details

namespace handle {
class CommandBuffer;
} // namespace handle;

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class CommandPool
{
  static_assert(details::is_transient_or_reset_only_v<pool_type>, "The only allowed values for create_flags are bit masks "
      "with zero or more VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and/or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.");

 public:
  constexpr static vk::CommandPoolCreateFlags create_flags = static_cast<vk::CommandPoolCreateFlags>(pool_type);

 private:
  LogicalDevice const* m_logical_device;        // Pointer to the associated logical device.
  vk::UniqueCommandPool m_command_pool;         // Command pool handle.

 public:
  // Construct a CommandPool.
  CommandPool(LogicalDevice const* logical_device, QueueFamilyPropertiesIndex queue_family
      COMMA_CWDEBUG_ONLY(Ambifix const& debug_name))
    : m_logical_device(logical_device),
      m_command_pool(logical_device->create_command_pool(queue_family.get_value(), create_flags
          COMMA_CWDEBUG_ONLY(debug_name)))
  { }

  handle::CommandBuffer allocate_buffer(
      CWDEBUG_ONLY(Ambifix const& ambifix));

  void free_buffer(handle::CommandBuffer command_buffer);

  void free_buffers(uint32_t count, handle::CommandBuffer const* command_buffers);

  void allocate_buffers(uint32_t count, handle::CommandBuffer* command_buffers
      COMMA_CWDEBUG_ONLY(Ambifix const& debug_name));

  [[gnu::always_inline]] inline void allocate_buffers(std::vector<handle::CommandBuffer>& command_buffers
      COMMA_CWDEBUG_ONLY(Ambifix const& debug_name));

  template<size_t N>
  void allocate_buffers(std::array<handle::CommandBuffer, N>& command_buffers
      COMMA_CWDEBUG_ONLY(Ambifix const& debug_name))
  {
    allocate_buffers(static_cast<uint32_t>(N), command_buffers.data()
        COMMA_CWDEBUG_ONLY(debug_name));
  }
};

} // namespace vulkan

#include "CommandBuffer.h"
#endif // COMMAND_POOL_H

#ifndef COMMAND_POOL_DEFINITION_H
#define COMMAND_POOL_DEFINITION_H

namespace vulkan {

template<vk::CommandPoolCreateFlags::MaskType pool_type>
handle::CommandBuffer CommandPool<pool_type>::allocate_buffer(
    CWDEBUG_ONLY(Ambifix const& debug_name))
{
  handle::CommandBuffer command_buffer;
  m_logical_device->allocate_command_buffers(*m_command_pool, vk::CommandBufferLevel::ePrimary, 1, &command_buffer.m_vh_command_buffer
      COMMA_CWDEBUG_ONLY(debug_name, false));
  return command_buffer;
}

template<vk::CommandPoolCreateFlags::MaskType pool_type>
void CommandPool<pool_type>::allocate_buffers(uint32_t count, handle::CommandBuffer* command_buffers
    COMMA_CWDEBUG_ONLY(Ambifix const& debug_name))
{
  static_assert(sizeof(command_buffers[0]) == sizeof(command_buffers[0].m_vh_command_buffer), "m_vh_command_buffer must be the only member");
  m_logical_device->allocate_command_buffers(*m_command_pool, vk::CommandBufferLevel::ePrimary, count, command_buffers->get_array()
      COMMA_CWDEBUG_ONLY(debug_name, true));
}

//inline
template<vk::CommandPoolCreateFlags::MaskType pool_type>
void CommandPool<pool_type>::allocate_buffers(std::vector<handle::CommandBuffer>& command_buffers
    COMMA_CWDEBUG_ONLY(Ambifix const& debug_name))
{
  allocate_buffers(static_cast<uint32_t>(command_buffers.size()), command_buffers.data()
      COMMA_CWDEBUG_ONLY(debug_name));
}

template<vk::CommandPoolCreateFlags::MaskType pool_type>
void CommandPool<pool_type>::free_buffer(handle::CommandBuffer command_buffer)
{
  m_logical_device->free_command_buffers(*m_command_pool, 1, &command_buffer.m_vh_command_buffer);
}

template<vk::CommandPoolCreateFlags::MaskType pool_type>
void CommandPool<pool_type>::free_buffers(uint32_t count, handle::CommandBuffer const* command_buffers)
{
  static_assert(sizeof(command_buffers[0]) == sizeof(command_buffers[0].m_vh_command_buffer), "m_vh_command_buffer must be the only member");
  m_logical_device->free_command_buffers(*m_command_pool, count, command_buffers->get_array());
}

} // namespace vulkan

#endif // COMMAND_POOL_DEFINITION_H
