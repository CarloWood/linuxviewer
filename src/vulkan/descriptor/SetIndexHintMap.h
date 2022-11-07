#pragma once

#include "SetIndex.h"
#include <map>
#include "debug.h"

namespace vulkan::descriptor {

class SetIndexHintMap
{
 private:
  using set_map_container_t = utils::Vector<SetIndex, SetIndexHint>;
  set_map_container_t m_set_index_map;

 public:
  void add_from_to(descriptor::SetIndexHint sih1, descriptor::SetIndexHint sih2)
  {
    DoutEntering(dc::vulkan, "add_from_to(" << sih1 << ", " << sih2);
    descriptor::SetIndex final_set_index{sih2.get_value()};
    if (sih1 >= m_set_index_map.iend())
      m_set_index_map.resize(sih1.get_value() + 1);
    // Don't call add_from_to twice for the same sih1.
    ASSERT(m_set_index_map[sih1].undefined());
    m_set_index_map[sih1] = final_set_index;
  }

  void clear()
  {
    m_set_index_map.clear();
  }

  descriptor::SetIndex convert(descriptor::SetIndexHint set_index_hint) const { return m_set_index_map[set_index_hint]; }

  // Accessor.
  bool empty() const { return m_set_index_map.empty(); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
