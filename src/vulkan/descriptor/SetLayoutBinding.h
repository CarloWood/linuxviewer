#pragma once

#include "statefultask/TaskEvent.h"
#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace vulkan::descriptor {

class SetLayoutBinding
{
 private:
  vk::DescriptorSetLayout m_descriptor_set_layout;
  uint32_t m_binding;

 public:
  SetLayoutBinding(vk::DescriptorSetLayout descriptor_set_layout, uint32_t binding) : m_descriptor_set_layout(descriptor_set_layout), m_binding(binding) { }

  friend bool operator<(SetLayoutBinding const& lhs, SetLayoutBinding const& rhs)
  {
    if (lhs.m_descriptor_set_layout < rhs.m_descriptor_set_layout)
      return true;
    if (lhs.m_descriptor_set_layout == rhs.m_descriptor_set_layout &&
        lhs.m_binding < rhs.m_binding)
      return true;
    return false;
  }

  // Accessors.
  vk::DescriptorSetLayout descriptor_set_layout() const { return m_descriptor_set_layout; }
  uint32_t binding() const { return m_binding; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
