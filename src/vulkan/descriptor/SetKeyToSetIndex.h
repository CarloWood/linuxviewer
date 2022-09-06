#pragma once

#include "SetKey.h"
#include "SetIndex.h"
#include <map>

namespace vulkan::descriptor {

class SetKeyToSetIndex
{
 private:
  SetIndex m_next_set_index{size_t{0}};
  std::map<SetKey, SetIndex> m_set_key_to_set_index;

 public:
  SetIndex try_emplace(SetKey set_key)
  {
    DoutEntering(dc::vulkan, "SetKeyToSetIndex::try_emplace(" << set_key << ") [" << this << "]");
    auto res = m_set_key_to_set_index.try_emplace(set_key, m_next_set_index);
    if (res.second)
      ++m_next_set_index;
    Dout(dc::vulkan, "Returning SetIndex " << res.first->second);
    return res.first->second;
  }
};

} // namespace vulkan::identifier
