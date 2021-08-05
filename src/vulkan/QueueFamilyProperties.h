#pragma once

#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug_ostream_operators.h"
#include <iomanip>
#endif
#include "debug.h"

namespace vulkan {

class QueueFamilyProperties : public vk::QueueFamilyProperties
{
 private:
  bool m_has_presentation_support;

 public:
  QueueFamilyProperties(vk::QueueFamilyProperties const& queue_family_properties, bool has_presentation_support) :
    vk::QueueFamilyProperties(queue_family_properties), m_has_presentation_support(has_presentation_support)
  {
    DoutEntering(dc::vulkan, "QueueFamilyProperties(" << queue_family_properties << ", has_presentation_support:" << std::boolalpha << has_presentation_support << ")");
  }

  bool has_presentation_support() const
  {
    return m_has_presentation_support;
  }
};

} // namespace vulkan
