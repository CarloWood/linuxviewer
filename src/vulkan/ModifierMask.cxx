#include "sys.h"
#include "ModifierMask.h"
#include <iostream>

namespace vulkan {

std::string ModifierMask::to_string() const
{
  static char const* mods[] = {
    "Shift", "Lock", "Ctrl", "Alt", "Super"
  };
  if (m_mask == 0)
    return "None";
  char const** mod;
  std::string result;
  char const* prefix = "";
  uint32_t mask = m_mask;
  for (mod = mods ; mask; mask >>= 1, mod++)
  {
    if ((mask & 1))
    {
      result += prefix;
      result += *mod;
      prefix = "|";
    }
  }
  return result;
}

std::ostream& operator<<(std::ostream& os, ModifierMask modifiers)
{
  os << modifiers.to_string();
  return os;
}

} // namespace vulkan
