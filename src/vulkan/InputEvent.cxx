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

std::ostream& operator<<(std::ostream& os, MousePosition mouse_position)
{
  os << '{';
  os << mouse_position.x() << ", " << mouse_position.y();
  return os << '}';
}

char const* to_string(EventType event_type)
{
  switch (event_type)
  {
    AI_CASE_RETURN(EventType::key_release);
    AI_CASE_RETURN(EventType::key_press);
    AI_CASE_RETURN(EventType::button_release);
    AI_CASE_RETURN(EventType::button_press);
    AI_CASE_RETURN(EventType::window_leave);
    AI_CASE_RETURN(EventType::window_enter);
    AI_CASE_RETURN(EventType::window_out_focus);
    AI_CASE_RETURN(EventType::window_in_focus);
  }
  AI_NEVER_REACHED
}

std::ostream& operator<<(std::ostream& os, EventType event_type)
{
  return os << to_string(event_type);
}

std::ostream& operator<<(std::ostream& os, ButtonsEventType button_events)
{
  os << '{';
  os << "mouse_buttons:" << button_events.buttons() <<
      ", event_type:" << button_events.event_type();
  return os << '}';
}

std::ostream& operator<<(std::ostream& os, InputEvent const& input_event)
{
  os << '{';
  os << "mouse_position:" << input_event.mouse_position <<
      ", modifier_mask:" << input_event.modifier_mask <<
      ", flags:" << input_event.flags;
  if (input_event.flags.event_type() == EventType::key_release || input_event.flags.event_type() == EventType::key_press)
    os << ", keysym:" << input_event.keysym;
  else if (input_event.flags.event_type() == EventType::button_release || input_event.flags.event_type() == EventType::button_press)
    os << ", button:" << (int)input_event.button;
  return os << '}';
}

#endif

} // namespace vulkan
