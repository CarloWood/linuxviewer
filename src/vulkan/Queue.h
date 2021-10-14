#pragma once

#include "QueueFamilyProperties.h"
#include <vulkan/vulkan.hpp>

namespace vulkan {

class Queue
{
  vk::Queue m_vh_queue;
  QueueFamilyPropertiesIndex m_queue_family;

 public:
  Queue() = default;
  Queue(vk::Queue vh_queue, QueueFamilyPropertiesIndex queue_family_index) : m_vh_queue(vh_queue), m_queue_family(queue_family_index)
  {
    // Both need to be either set or not set.
    ASSERT(static_cast<bool>(vh_queue) != queue_family_index.undefined());
  }

  // Accessors.
  operator vk::Queue() const { return m_vh_queue; }
  QueueFamilyPropertiesIndex queue_family() const { return m_queue_family; }
  operator bool() const { return !m_queue_family.undefined(); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan
