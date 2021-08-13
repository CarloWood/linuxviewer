#pragma once

#include "QueueRequest.h"       // QueueRequestIndex
#include "Device.h"
#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vulkan {

struct CommandPoolCreateInfo : vk::CommandPoolCreateInfo
{
#ifdef CWDEBUG
  static constexpr char const* default_debug_name = "CommandPool";
#endif

 private:
  QueueRequestIndex m_queue_request_index;      // A cache for information that is used in setup.
#ifdef CWDEBUG
  std::string m_debug_name = default_debug_name;
#endif

 public:
  CommandPoolCreateInfo& setQueueRequestIndex(QueueRequestIndex queue_request_index)
  {
    m_queue_request_index = queue_request_index;
    return *this;
  }

  void setup(vulkan::Device const& device)
  {
    setQueueFamilyIndex(device.get_queue_family(m_queue_request_index).get_value());
  }

#ifdef CWDEBUG
  // Setter for debug name.
  CommandPoolCreateInfo& setDebugName(std::string debug_name)
  {
    m_debug_name = std::move(debug_name);
    return *this;
  }
#endif

#ifdef CWDEBUG
  char const* debug_name() const
  {
    return m_debug_name.c_str();
  }

  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
