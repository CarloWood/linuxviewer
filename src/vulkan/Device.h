#pragma once

#include <vulkan/vulkan.hpp>
#include "QueueRequest.h"       // For QueueRequestIndex.
#include "QueueReply.h"
#include "utils/Vector.h"
#include "debug.h"
#ifdef CWDEBUG
#include <iosfwd>
#include <string>
#endif

namespace vulkan {

struct DeviceCreateInfo;
struct CommandPoolCreateInfo;
class ExtensionLoader;

class Device
{
 private:
  vk::PhysicalDevice m_physical_device;         // The underlaying physical device.
  vk::UniqueDevice m_device_handle;             // A handle to the logical device.
  utils::Vector<QueueReply, QueueRequestIndex> m_queue_replies;
  vk::UniqueCommandPool m_command_pool;

 public:
  Device() = default;                           // Must be initialized by calling setup.
  ~Device();
  // Not copyable or movable.
  Device(Device const&) = delete;
  void operator=(Device const&) = delete;

  void setup(vk::Instance vulkan_instance, ExtensionLoader& extension_loader, vk::SurfaceKHR surface, DeviceCreateInfo&& device_create_info);

  uint32_t number_of_queues(QueueRequestIndex request_index) const
  {
    QueueReply const& reply = m_queue_replies[request_index];
    return reply.number_of_queues();
  }

  vk::Queue get_queue_handle(QueueRequestIndex request_index, uint32_t queue_index) const
  {
    QueueReply const& reply = m_queue_replies[request_index];
    // Use number_of_queues() to find out what the largest possible index is.
    ASSERT(queue_index < reply.number_of_queues());
    return m_device_handle->getQueue(reply.get_queue_family_handle().get_value(), queue_index);
  }

  GPU_queue_family_handle get_queue_family(QueueRequestIndex request_index) const
  {
    ASSERT(request_index < m_queue_replies.iend());
    return m_queue_replies[request_index].get_queue_family_handle();
  }

  void create_command_pool(CommandPoolCreateInfo const& command_pool_create_info);

  // Member functions needed to make HelloTriangleSwapChain happy.
  vk::Device device() const
  {
    return *m_device_handle;
  }

  vk::PhysicalDevice get_physical_device() const
  {
    return m_physical_device;
  }

  vk::CommandPool get_command_pool() const
  {
    return *m_command_pool;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
