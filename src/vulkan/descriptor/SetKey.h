#pragma once

#include "utils/UniqueID.h"
#ifdef CWDEBUG
#include "debug/vulkan_print_on.h"
#endif
#include "debug.h"

namespace vulkan::descriptor {
class SetKeyContext;

// SetKey
//
// Each key is a unique identifier that gives access to a descriptor set,
// in the context of a given ShaderInputData, where two different keys
// can refer to the same descriptor set even though they are different.
//
class SetKey
{
 private:
  static utils::UniqueIDContext<size_t> s_dummy_context;        // Not really serving sensible values.
  utils::UniqueID<size_t> m_id;
#ifdef CWDEBUG
  bool m_debug_id_is_set{};
#endif

 public:
  // A default constructed SetKey *must* be assigned a value later with a (move) assignment!
  SetKey() : m_id(s_dummy_context.get_id()) { }

  SetKey(SetKeyContext& descriptor_set_key_context);

  // Accessor.
  utils::UniqueID<size_t> id() const
  {
    // Do not call id() on a default constructed SetKey that wasn't assigned value with an id yet.
    ASSERT(m_debug_id_is_set);
    return m_id;
  }

  // SetKey is used as a key in a std::map (SetKeyToSetIndexHint).
  // Add an ordering.
  bool operator<(SetKey const& rhs) const
  {
    return m_id < rhs.m_id;
  }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
