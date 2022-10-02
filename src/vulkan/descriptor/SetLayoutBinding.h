#pragma once

#include <vulkan/vulkan.hpp>
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif

namespace vulkan::descriptor {
class SetLayout;

class SetLayoutBinding
{
 private:
  SetLayout const* m_set_layout_canonical_ptr;
  uint32_t m_binding;
  mutable vk::DescriptorSet m_handle;   // Not used as part of the ordering key.

 public:
  // Construct a SetLayoutBinding; handle might be VK_NULL_HANDLE in which case it is set later with set_handle.
  SetLayoutBinding(SetLayout const* set_layout_canonical_ptr, uint32_t binding, vk::DescriptorSet handle) : m_set_layout_canonical_ptr(set_layout_canonical_ptr), m_binding(binding), m_handle(handle) { }

  void set_handle(vk::DescriptorSet handle) const
  {
    // Only call this function once.
    ASSERT(!m_handle);
    m_handle = handle;
  }

  friend bool operator<(SetLayoutBinding const& lhs, SetLayoutBinding const& rhs)
  {
    if (lhs.m_set_layout_canonical_ptr < rhs.m_set_layout_canonical_ptr)
      return true;
    if (lhs.m_set_layout_canonical_ptr == rhs.m_set_layout_canonical_ptr &&
        lhs.m_binding < rhs.m_binding)
      return true;
    return false;
  }

  // Accessors.
  SetLayout const* set_layout_canonical_ptr() const { return m_set_layout_canonical_ptr; }
  uint32_t binding() const { return m_binding; }
  vk::DescriptorSet handle() const { return m_handle; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
