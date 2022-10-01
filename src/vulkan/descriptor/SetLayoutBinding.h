#pragma once

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

 public:
  SetLayoutBinding(SetLayout const* set_layout_canonical_ptr, uint32_t binding) : m_set_layout_canonical_ptr(set_layout_canonical_ptr), m_binding(binding) { }

  // Accessors.
  SetLayout const* set_layout_canonical_ptr() const { return m_set_layout_canonical_ptr; }
  uint32_t binding() const { return m_binding; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
