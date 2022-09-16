#pragma once

#include "descriptor/SetIndex.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace vulkan::descriptor {

class SetBinding
{
 private:
  SetIndexHint m_set_index_hint;
  int m_binding;

 public:
  SetBinding(SetIndexHint set_index_hint, int binding) : m_set_index_hint(set_index_hint), m_binding(binding) { }

  // Accessors.
  SetIndexHint set_index_hint() const { return m_set_index_hint; }
  int binding() const { return m_binding; }

  // This is used as a key in a std::map.
  friend bool operator<(SetBinding const& lhs, SetBinding const& rhs)
  {
    if (lhs.m_set_index_hint != rhs.m_set_index_hint)
      return lhs.m_set_index_hint < rhs.m_set_index_hint;
    return lhs.m_binding < rhs.m_binding;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
