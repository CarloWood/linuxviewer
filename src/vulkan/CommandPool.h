#ifndef COMMAND_POOL_H
#define COMMAND_POOL_H

#include "LogicalDevice.h"
#include "QueueFamilyProperties.h"
#include "threadsafe/aithreadsafe.h"
#include <vector>

namespace vulkan {
namespace details {

template<uint32_t flags>
struct is_transient_or_reset
{
  constexpr static bool value = (flags & (VK_COMMAND_POOL_CREATE_TRANSIENT_BIT|VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT)) == flags;
};

template<uint32_t flags>
inline constexpr bool is_transient_or_reset_v = is_transient_or_reset<flags>::value;

#ifdef CWDEBUG
using UniquePoolID = uint64_t;
UniquePoolID get_unique_pool_id();
#endif

} // namespace details

namespace handle {
class CommandBuffer;
} // namespace handle;

template<vk::CommandPoolCreateFlags::MaskType pool_type>
class UnlockedCommandPool
{
  static_assert(details::is_transient_or_reset_v<pool_type>, "The only allowed values for create_flags are bits masks "
      "with zero or more VK_COMMAND_POOL_CREATE_TRANSIENT_BIT and/or VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.");

  constexpr static vk::CommandPoolCreateFlags create_flags = static_cast<vk::CommandPoolCreateFlags>(pool_type);

 private:
  LogicalDevice const* m_logical_device;        // Pointer to the associated logical device.
  vk::UniqueCommandPool m_command_pool;         // Command pool handle.
#ifdef CWDEBUG
  details::UniquePoolID m_id = details::get_unique_pool_id();
#endif

 public:
  UnlockedCommandPool() = default;

  // Construct an UnlockedCommandPool.
  UnlockedCommandPool(LogicalDevice const* logical_device, QueueFamilyPropertiesIndex queue_family
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name))
    : m_logical_device(logical_device),
      m_command_pool(logical_device->create_command_pool(queue_family.get_value(), create_flags
          COMMA_CWDEBUG_ONLY(debug_name)))
  { }

#ifdef CWDEBUG
  // Accessor.
  details::UniquePoolID id() const { return m_id; }
#endif

  handle::CommandBuffer allocate_buffer(
      CWDEBUG_ONLY(AmbifixOwner const& ambifix));

  void allocate_buffers(uint32_t count, handle::CommandBuffer* command_buffers
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name));

  void allocate_buffers(std::vector<handle::CommandBuffer>& command_buffers
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name))
  {
    allocate_buffers(static_cast<uint32_t>(command_buffers.size()), command_buffers.data()
        COMMA_CWDEBUG_ONLY(debug_name));
  }

  template<size_t N>
  void allocate_buffers(std::array<handle::CommandBuffer, N>& command_buffers
      COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name))
  {
    allocate_buffers(static_cast<uint32_t>(N), command_buffers.data()
        COMMA_CWDEBUG_ONLY(debug_name));
  }
};

template<vk::CommandPoolCreateFlags::MaskType pool_type = 0>
using CommandPool = aithreadsafe::Wrapper<UnlockedCommandPool<pool_type>, aithreadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan

#include "CommandBuffer.h"
#endif // COMMAND_POOL_H

#ifndef COMMAND_POOL_DEFINITION_H
#define COMMAND_POOL_DEFINITION_H

namespace vulkan {

template<vk::CommandPoolCreateFlags::MaskType pool_type>
handle::CommandBuffer UnlockedCommandPool<pool_type>::allocate_buffer(
    CWDEBUG_ONLY(AmbifixOwner const& debug_name))
{
  handle::CommandBuffer command_buffer{CWDEBUG_ONLY(m_id)};
  m_logical_device->allocate_command_buffers(*m_command_pool, vk::CommandBufferLevel::ePrimary, 1, &command_buffer.m_handle
      COMMA_CWDEBUG_ONLY(debug_name, false));
  return command_buffer;
}

template<vk::CommandPoolCreateFlags::MaskType pool_type>
void UnlockedCommandPool<pool_type>::allocate_buffers(uint32_t count, handle::CommandBuffer* command_buffers
    COMMA_CWDEBUG_ONLY(AmbifixOwner const& debug_name))
{
#ifndef CWDEBUG
  static_assert(sizeof(command_buffers[0]) == sizeof(command_buffers[0].m_handle), "m_handle must be the only member");
  m_logical_device->allocate_command_buffers(*m_command_pool, vk::CommandBufferLevel::ePrimary, count, &command_buffers[0].m_handle);
#else
  std::vector<vk::CommandBuffer> tmp(count);
  m_logical_device->allocate_command_buffers(*m_command_pool, vk::CommandBufferLevel::ePrimary, count, tmp.data(), debug_name);
  for (int i = 0; i < count; ++i)
  {
    command_buffers[i].m_handle = tmp[i];
    command_buffers[i].m_pool_id = m_id;
    DebugSetName(command_buffers[i].m_handle, debug_name("[" + to_string(i) + "]"));
  }
#endif
}

} // namespace vulkan

#endif // COMMAND_POOL_DEFINITION_H
