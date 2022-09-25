#pragma once

#include "SetKey.h"
#include "SetIndex.h"
#include <map>

namespace vulkan::descriptor {

class SetKeyToSetIndexHint
{
 private:
  SetIndexHint m_next_set_index_hint{size_t{0}};
  std::map<SetKey, SetIndexHint> m_set_key_to_set_index_hint;

 public:
  SetIndexHint try_emplace_set_index_hint(SetKey set_key)
  {
    DoutEntering(dc::vulkan, "SetKeyToSetIndexHint::try_emplace(" << set_key << ") [" << this << "]");
    auto res = m_set_key_to_set_index_hint.try_emplace(set_key, m_next_set_index_hint);
    if (res.second)
      ++m_next_set_index_hint;
    Dout(dc::vulkan, "Returning SetIndex " << res.first->second);
    return res.first->second;
  }

  SetIndexHint get_set_index_hint(SetKey set_key) const
  {
    auto set_index_hint = m_set_key_to_set_index_hint.find(set_key);
    // Don't call get_set_index_hint for a key that wasn't first passed to try_emplace_set_index_hint.
    ASSERT(set_index_hint != m_set_key_to_set_index_hint.end());
    return set_index_hint->second;
  }
};

} // namespace vulkan::identifier
