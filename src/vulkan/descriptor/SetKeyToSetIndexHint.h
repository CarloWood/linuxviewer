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
  // Insert a known set index hint.
  void try_emplace_set_index_hint(SetKey set_key, SetIndexHint set_index_hint)
  {
    DoutEntering(dc::vulkan, "SetKeyToSetIndexHint::try_emplace(" << set_key << ", " << set_index_hint << ") [" << this << "]");
    auto res = m_set_key_to_set_index_hint.try_emplace(set_key, set_index_hint);
    // Do not add the same set_key twice.
    ASSERT(res.second);
  }

  // Generate a new set index hint.
  SetIndexHint try_emplace_set_index_hint(SetKey set_key)
  {
    DoutEntering(dc::vulkan, "SetKeyToSetIndexHint::try_emplace(" << set_key << ") [" << this << "]");
    auto res = m_set_key_to_set_index_hint.try_emplace(set_key, m_next_set_index_hint);
    // Do not add the same set_key twice.
    ASSERT(res.second);
//    if (res.second)    FIXME: remove this if (see the above ASSERT).
      ++m_next_set_index_hint;
    Dout(dc::vulkan, "Returning SetIndex " << res.first->second);
    return res.first->second;
  }

  SetIndexHint get_set_index_hint(SetKey set_key) const
  {
    DoutEntering(dc::vulkan, "SetKeyToSetIndexHint::get_set_index_hint(" << set_key << ") [" << this << "]");
    auto set_index_hint = m_set_key_to_set_index_hint.find(set_key);
    // Don't call get_set_index_hint for a key that wasn't first passed to try_emplace_set_index_hint.
    ASSERT(set_index_hint != m_set_key_to_set_index_hint.end());
    return set_index_hint->second;
  }
};

} // namespace vulkan::identifier
