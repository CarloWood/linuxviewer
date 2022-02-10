#pragma once

#include "utils/threading/FIFOBuffer.h"
#include "threadsafe/aithreadsafe.h"
#include <iosfwd>
#include <cstdint>
#include <atomic>
#include <limits>
#include "debug.h"

namespace vulkan {

// Mapping of modifier keys to the same values as used by imgui.
// Extended with Lock (not used by imgui).
//
// This class is single threaded; only called from EventThread.
struct ModifierMask
{
  static constexpr uint16_t Ctrl    = 1 << 0;        // Both, left and/or right.
  static constexpr uint16_t Shift   = 1 << 1;        // Both, left and/or right.
  static constexpr uint16_t Alt     = 1 << 2;        // Both, left and/or right.
  static constexpr uint16_t Super   = 1 << 3;
  static constexpr uint16_t Lock    = 1 << 4;        // When pressed or CapsLock on.

  static constexpr uint32_t unused  = 1 << 5;        // First unused bit; used by AtomicInputState below.

#if 0   // This assert was moved to ImGui.cxx because I don't want to #include <imgui.h> here.
  // vulkan::Window::convert should map XCB codes to our codes, which in turn must be equal to what imgui uses.
  static_assert(
      Ctrl  == ImGuiKeyModFlags_Ctrl &&
      Shift == ImGuiKeyModFlags_Shift &&
      Alt   == ImGuiKeyModFlags_Alt &&
      Super == ImGuiKeyModFlags_Super, "Modifier mapping needs fixing.");
#endif

  static constexpr uint16_t imgui_mask = Ctrl|Shift|Alt|Super;

 private:
  uint16_t m_mask;

 public:
  ModifierMask() : m_mask(0) { }
  ModifierMask(uint16_t mask) : m_mask(mask) { }
  ModifierMask(ModifierMask const&) = default;
  ModifierMask& operator=(ModifierMask const&) = default;

  uint16_t get_value() const { return m_mask; }
  uint16_t imgui() const { return m_mask & imgui_mask; }

#ifdef CWDEBUG
  std::string to_string() const;
#endif
};

static_assert(std::is_trivially_copyable_v<ModifierMask>, "ModifierMask must be trivially copyable because we're going to use it in FIFOBuffer.");

std::ostream& operator<<(std::ostream& os, ModifierMask mask);

// Mapping of mouse button values as used by WindowEvents::on_mouse_click and ImGui::on_mouse_click.
// Note that xcb (X11) starts coutning from 0 and has Right and Middle swapped; but we deliberately use the same as imgui.
//
// This class is single threaded; only called from EventThread.
struct MouseButtons
{
  static constexpr int Left       = 0;
  static constexpr int Right      = 1;
  static constexpr int Middle     = 2;
  static constexpr int WheelUp    = 3;
  static constexpr int WheelDown  = 4;
  static constexpr int WheelLeft  = 5;
  static constexpr int WheelRight = 6;

  // Sorry, but here I use X11 names.
  static constexpr int Button8    = 7;
  static constexpr int Button9    = 8;
  static constexpr int Button10   = 9;
  static constexpr int Button11   = 10;

  static constexpr uint16_t wheelbits = (uint16_t{1} << WheelUp) | (uint16_t{1} << WheelDown) | (uint16_t{1} << WheelLeft) | (uint16_t{1} << WheelRight);
  static constexpr uint16_t allbits = 0x1fff;   // At most 13 bits may be used, see ButtonsEventType below.

  static uint16_t as_mask(uint8_t button)
  {
    ASSERT(button < 13);
    return uint16_t{1} << button;
  }

  static bool is_wheel(uint8_t button)
  {
    return as_mask(button) & wheelbits;
  }

 private:
  uint16_t m_mask;

 public:
  MouseButtons() : m_mask(0) { }
  MouseButtons(int mask) : m_mask(mask & allbits) { }
  MouseButtons(MouseButtons const&) = default;
  MouseButtons& operator=(MouseButtons const&) = default;

  void update_button(uint8_t button, bool pressed)
  {
    uint16_t mask = as_mask(button);
    if (pressed)
      m_mask |= mask;
    else
      m_mask &= ~mask;
  }

  uint16_t get_value() const { return m_mask; }

#ifdef CWDEBUG
  std::string to_string() const;
#endif
};

static_assert(std::is_trivially_copyable_v<MouseButtons>, "MouseButtons must be trivially copyable because we're going to use it in FIFOBuffer.");

std::ostream& operator<<(std::ostream& os, MouseButtons mask);

class MousePosition
{
  int16_t m_x;
  int16_t m_y;

 public:
  MousePosition() : m_x(std::numeric_limits<int16_t>::max()), m_y(std::numeric_limits<int16_t>::max()) { }
  MousePosition(int16_t x, int16_t y) : m_x(x), m_y(y) { }
  MousePosition(MousePosition const&) = default;
  MousePosition& operator=(MousePosition const&) = default;

  void set(int16_t x, int16_t y) { m_x = x; m_y = y; }

  int16_t x() const { return m_x; }
  int16_t y() const { return m_y; }
};

static_assert(std::is_trivially_copyable_v<MousePosition>, "MousePosition must be trivially copyable because we're going to use it in FIFOBuffer.");

enum class EventType : uint16_t         // Uses 3 bits, see ButtonsEventType.
{
  // The least significant bit means: pressed/clicked/entered/focus.
  key_release,
  key_press,
  button_release,
  button_press,
  window_leave,
  window_enter,
  window_out_focus,
  window_in_focus
};

struct ButtonsEventType
{
 private:
  uint16_t m_mouse_buttons:13;
  EventType m_event_type:3;

 public:
  ButtonsEventType(MouseButtons mouse_buttons, EventType event_type) : m_mouse_buttons(mouse_buttons.get_value()), m_event_type(event_type) { }

  MouseButtons buttons() const { return static_cast<MouseButtons>(m_mouse_buttons); }
  EventType event_type() const { return m_event_type; }
};

struct InputEvent
{
  MousePosition mouse_position;
  ModifierMask modifier_mask;
  ButtonsEventType flags;
  union {
    uint32_t keysym;
    uint8_t button;
  };
};

static_assert(std::is_trivially_copyable_v<InputEvent>, "InputEvent must be trivially copyable because we're going to use it in FIFOBuffer.");

#ifdef CWDEBUG
std::ostream& operator<<(std::ostream& os, InputEvent const& input_event);
#endif

using InputEventBuffer = utils::threading::FIFOBuffer<1, InputEvent>;

struct UnlockedWheelOffset
{
  float x;
  float y;

  void accumulate_x(float x_inc)
  {
    x += x_inc;
  }

  void accumulate_y(float y_inc)
  {
    y += y_inc;
  }

  void consume(float& delta_x_out, float& delta_y_out)
  {
    delta_x_out = x;
    delta_y_out = y;
    x = y = 0.f;
  }
};

using WheelOffset = aithreadsafe::Wrapper<UnlockedWheelOffset, aithreadsafe::policy::Primitive<std::mutex>>;
using MovedMousePosition = aithreadsafe::Wrapper<MousePosition, aithreadsafe::policy::Primitive<std::mutex>>;

} // namespace vulkan
