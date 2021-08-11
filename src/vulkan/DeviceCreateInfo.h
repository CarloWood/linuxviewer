#pragma once

#include "PhysicalDeviceFeatures.h"
#include "QueueRequest.h"
#include "utils/Array.h"
#include <iosfwd>
#include "debug.h"

namespace vulkan {

struct DeviceCreateInfo : vk::DeviceCreateInfo
{
  static constexpr utils::Array<QueueRequest, 2> default_queue_requests = {{{
    { .queue_flags = QueueFlagBits::eGraphics, .max_number_of_queues = 1 },
    { .queue_flags = QueueFlagBits::ePresentation, .max_number_of_queues = 1 }
  }}};
#ifdef CWDEBUG
  // This name reflects the usual place where the handle to the device will be stored.
  static constexpr char const* default_debug_name = "Application::m_vulkan_device";
#endif

 private:
  utils::Vector<QueueRequest> m_queue_requests = {};    // Required queue flags. The default is used when this is empty.
  QueueFlags m_queue_flags = QueueFlagBits::none;       // Bitwise-OR of all queue_flags in m_queue_requests.
  std::vector<char const*> m_device_extensions;
#ifdef CWDEBUG
  std::string m_debug_name = default_debug_name;
#endif

 public:
  DeviceCreateInfo(vulkan::PhysicalDeviceFeatures const& physical_device_features = vulkan::PhysicalDeviceFeatures::s_default_physical_device_features)
  {
    setPEnabledFeatures(&physical_device_features);
  }

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
  utils::Vector<QueueRequest>& get_queue_requests()
  {
    // Don't call this function.
    ASSERT(m_queue_requests.empty());
    return m_queue_requests;
  }

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

#ifdef CWDEBUG
  char const* debug_name() const
  {
    return m_debug_name.c_str();
  }

  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
