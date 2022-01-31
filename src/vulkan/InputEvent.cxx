#include "sys.h"
#include "InputEvent.h"
#include <iostream>

namespace vulkan {

#ifdef CWDEBUG
namespace  {

std::string bit_mask_to_string(char const** mods, uint16_t mask)
{
  if (mask == 0)
    return "None";
  char const** mod;
  std::string result;
  char const* prefix = "";
  for (mod = mods ; mask; mask >>= 1, ++mod)
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

} // namespace

std::string ModifierMask::to_string() const
{
  static char const* mods[] = {
    "Ctrl", "Shift", "Alt", "Super", "Lock"
  };
  return bit_mask_to_string(mods, m_mask);
}

std::string MouseButtons::to_string() const
{
  static char const* mods[] = {
    "Left", "Right", "Middle", "WheelUp", "WheelDown", "WheelLeft", "WheelRight", "Button8", "Button9", "Button10", "Button11"
  };
  return bit_mask_to_string(mods, m_mask);
}

std::ostream& operator<<(std::ostream& os, ModifierMask modifiers)
{
  os << modifiers.to_string();
  return os;
}

std::ostream& operator<<(std::ostream& os, MouseButtons mouse_buttons)
{
  os << mouse_buttons.to_string();
  return os;
}
#endif

} // namespace vulkan
