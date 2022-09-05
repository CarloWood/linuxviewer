#pragma once

#include "SetKey.h"

namespace vulkan::descriptor {

class SetKeyPreference
{
 private:
  SetKey const& m_set_key;
  float m_preference;

 public:
  SetKeyPreference(SetKey const& set_key, float preference) : m_set_key(set_key), m_preference(preference) { }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
