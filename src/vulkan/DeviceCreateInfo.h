#pragma once

#include "Defaults.h"
#include "utils/Vector.h"
#include <vector>
#ifdef CWDEBUG
#include <string>
#endif

namespace vulkan {

class DeviceCreateInfo : protected vk_defaults::DeviceCreateInfo
{
 private:
  utils::Vector<QueueRequest> m_queue_requests = {};    // Required queue flags. The default is used when this is empty.
  QueueFlags m_queue_flags = QueueFlagBits::none;       // Bitwise-OR of all queue_flags in m_queue_requests.
  std::vector<char const*> m_device_extensions;
#ifdef CWDEBUG
  std::string m_debug_name = default_debug_name;
#endif

 public:
  using vk_defaults::DeviceCreateInfo::DeviceCreateInfo;

  // Setter for required queue flags.
  DeviceCreateInfo& addQueueRequests(QueueRequest queue_request)
  {
    m_queue_requests.push_back(queue_request);
    m_queue_flags |= queue_request.queue_flags;
    return *this;
  }

  DeviceCreateInfo& addDeviceExtentions(vk::ArrayProxy<char const* const> extra_device_extensions);

#ifdef CWDEBUG
  // Setter for debug name.
  DeviceCreateInfo& setDebugName(std::string debug_name)
  {
    m_debug_name = std::move(debug_name);
    return *this;
  }
#endif

  // Used to set the default.
  void set_default_queue_requests()
  {
    // Don't call this function twice, or when addQueueRequests was already called by the user.
    ASSERT(m_queue_requests.empty());
    std::for_each(default_queue_requests.begin(), default_queue_requests.end(), [this](QueueRequest const& queue_request){ addQueueRequests(queue_request); });
  }

  // Accessor.
  utils::Vector<QueueRequest> const& get_queue_requests() const
  {
    return m_queue_requests;
  }

  QueueFlags get_queue_flags() const
  {
    return m_queue_flags;
  }

  bool has_queue_flag(QueueFlagBits queue_flag) const
  {
    return !!(m_queue_flags & queue_flag);
  }

  std::vector<char const*> const& device_extensions() const
  {
    return m_device_extensions;
  }

#ifdef CWDEBUG
  char const* debug_name() const
  {
    return m_debug_name.c_str();
  }

  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
