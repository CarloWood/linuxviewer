#pragma once

#include "descriptor/SetIndex.h"
#include "debug.h"

namespace vulkan::descriptor {

class SetBinding
{
 public:
  static constexpr uint32_t undefined_binding = 0xffffffff;

 private:
  SetIndexHint m_set_index_hint;
  uint32_t m_binding;

 public:
  SetBinding(SetIndexHint set_index_hint, uint32_t binding = undefined_binding) : m_set_index_hint(set_index_hint), m_binding(binding) { }

  void set_binding(uint32_t binding)
  {
    ASSERT(m_binding == undefined_binding);
    m_binding = binding;
  }

  // Accessors.
  SetIndexHint set_index_hint() const { return m_set_index_hint; }
  uint32_t binding() const { ASSERT(m_binding != undefined_binding); return m_binding; }

  // This is used as a key in a std::map.
  friend bool operator<(SetBinding const& lhs, SetBinding const& rhs)
  {
    ASSERT(lhs.m_binding != undefined_binding && rhs.m_binding != undefined_binding);
    if (lhs.m_set_index_hint != rhs.m_set_index_hint)
      return lhs.m_set_index_hint < rhs.m_set_index_hint;
    return lhs.m_binding < rhs.m_binding;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
