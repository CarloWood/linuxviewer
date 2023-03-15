#pragma once

#include "SetIndex.h"
#include "utils/AIAlert.h"
#include <map>
#include "debug.h"

namespace vulkan::descriptor {

class SetIndexHintMap
{
 private:
  using set_map_container_t = utils::Vector<SetIndex, SetIndexHint>;
  set_map_container_t m_set_index_map;

 public:
  void add_from_to(SetIndexHint sih1, SetIndexHint sih2)
  {
    DoutEntering(dc::vulkan, "add_from_to(" << sih1 << ", " << sih2 << ')');
    SetIndex final_set_index{sih2.get_value()};
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

  SetIndex convert(SetIndexHint set_index_hint) const
  {
    DoutEntering(dc::setindexhint|continued_cf, "SetIndexHintMap::convert(" << set_index_hint << ") = ");
    // If the value of set_index_hint was not passed to add_from_to as first argument, then
    // we should return an 'undefined' SetIndex. This then indicates that this set_index_hint
    // belongs to a shader resource that wasn't used in any of the shaders of the current pipeline.
    SetIndex result;
    if (set_index_hint < m_set_index_map.iend())
      result = m_set_index_map[set_index_hint];
    Dout(dc::finish, result);
    return result;
  }

  SetIndexHint reverse_convert(SetIndex set_index) const
  {
    DoutEntering(dc::setindexhint|continued_cf, "SetIndexHintMap::reverse_convert(" << set_index << ") = ");
    SetIndexHint result;
    for (SetIndexHint set_index_hint = m_set_index_map.ibegin(); set_index_hint != m_set_index_map.iend(); ++set_index_hint)
      if (m_set_index_map[set_index_hint] == set_index)
      {
        result = set_index_hint;
        break;
      }
    Dout(dc::finish, result);
    //FIXME: what if we don't find it?
    ASSERT(!result.undefined());
    return result;
  }

  // Accessor.
  bool empty() const { return m_set_index_map.empty(); }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
