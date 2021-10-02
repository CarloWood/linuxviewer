#pragma once

#include "QueueFlags.h"
#include "utils/Vector.h"
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include <iomanip>
#endif
#include "debug.h"

namespace vulkan {

class QueueFamilyProperties : public vk::QueueFamilyProperties
{
 private:
  QueueFlags m_queue_flags;

 public:
  QueueFamilyProperties(vk::QueueFamilyProperties const& queue_family_properties, bool has_presentation_support) :
    vk::QueueFamilyProperties(queue_family_properties),
    m_queue_flags(queue_family_properties.queueFlags | (has_presentation_support ? QueueFlagBits::ePresentation : QueueFlagBits::none))
  {
    DoutEntering(dc::vulkan, "QueueFamilyProperties(" << queue_family_properties << ", has_presentation_support:" << std::boolalpha << has_presentation_support << ")");
  }

  QueueFlags get_queue_flags() const
  {
    return m_queue_flags;
  }
};

// Queue family handles (uint32_t) are the index into the vector returned
// by vk::PhysicalDevice::getQueueFamilyProperties().
using QueueFamilyPropertiesIndex = utils::VectorIndex<QueueFamilyProperties>;

} // namespace vulkan
