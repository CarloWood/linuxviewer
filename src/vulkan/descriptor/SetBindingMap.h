#pragma once

#include "SetBinding.h"
#include "SetIndex.h"
#include <map>
#include "debug.h"

namespace vulkan::descriptor {

class SetBindingMap
{
 private:
  struct SetData
  {
    SetIndex m_set_index;
    using binding_map_container_t = std::vector<uint32_t>;
    binding_map_container_t m_binding_map;

    void add_from_to(uint32_t b1, uint32_t b2)
    {
      if (b1 >= m_binding_map.size())
        m_binding_map.resize(b1 + 1, 0xffffffff);
      // Don't call add_from_to twice for the same b1.
      ASSERT(m_binding_map[b1] == 0xffffffff);
      m_binding_map[b1] = b2;
    }
  };

  using set_map_container_t = utils::Vector<SetData, SetIndexHint>;
  set_map_container_t m_set_map;

 public:
  void add_from_to(descriptor::SetIndexHint sih1, descriptor::SetIndexHint sih2)
  {
    DoutEntering(dc::vulkan, "add_from_to(" << sih1 << ", " << sih2);
    descriptor::SetIndex final_set_index{sih2.get_value()};
    if (sih1 >= m_set_map.iend())
      m_set_map.resize(sih1.get_value() + 1);
    // Don't call add_from_to twice for the same sih1.
    ASSERT(m_set_map[sih1].m_set_index.undefined());
    m_set_map[sih1].m_set_index = final_set_index;
  }

  void add_from_to(descriptor::SetBinding sb1, uint32_t b2)
  {
    m_set_map[sb1.set_index_hint()].add_from_to(sb1.binding(), b2);
  }

  void clear()
  {
    m_set_map.clear();
  }

  descriptor::SetIndex convert(descriptor::SetIndexHint set_index_hint) const { return m_set_map[set_index_hint].m_set_index; }
  uint32_t convert(descriptor::SetIndexHint set_index_hint, uint32_t binding) const { return m_set_map[set_index_hint].m_binding_map[binding]; }

  // Accessor.
  bool empty() const { return m_set_map.empty(); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
