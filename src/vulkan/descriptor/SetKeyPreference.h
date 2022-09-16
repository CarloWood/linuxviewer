#pragma once

#include "SetKey.h"

namespace vulkan::descriptor {

class SetKeyPreference
{
 private:
  SetKey const& m_descriptor_set_key;
  float m_preference;

 public:
  SetKeyPreference(SetKey const& descriptor_set_key, float preference) : m_descriptor_set_key(descriptor_set_key), m_preference(preference) { }

  // Accessors.
  SetKey const& descriptor_set_key() const { return m_descriptor_set_key; }
  float preference() const { return m_preference; }

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};

} // namespace vulkan::descriptor
