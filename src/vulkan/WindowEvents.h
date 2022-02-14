#pragma once

#include "OperatingSystem.h"
#include "SpecialCircumstances.h"
#include "InputEvent.h"
#include "ImGui.h"

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct xcb;
extern channel_ct xcbmotion;
NAMESPACE_DEBUG_CHANNELS_END
#endif

namespace vulkan {

// Gives asynchronous access to SpecialCircumstances.
class AsyncAccessSpecialCircumstances
{
 private:
  SpecialCircumstances* m_special_circumstances = nullptr;      // Final value is set with set_special_circumstances.

 protected:
  void set_extent_changed() const        { m_special_circumstances->set_extent_changed(); }
  void set_must_close() const            { m_special_circumstances->set_must_close(); }
  void set_mapped() const                { m_special_circumstances->set_mapped(); }
  void set_unmapped() const              { m_special_circumstances->set_unmapped(); }

 public:
  // Called one time, immediately after (default) construction.
  void set_special_circumstances(SpecialCircumstances* special_circumstances)
  {
    // Don't call this function yourself.
    ASSERT(!m_special_circumstances);
    m_special_circumstances = special_circumstances;
  }
};

// Asychronous window events.
class WindowEvents : public linuxviewer::OS::Window, public AsyncAccessSpecialCircumstances
{
 private:
  MouseButtons m_mouse_buttons;                         // Cache of current mouse button state.
  InputEventBuffer* m_input_event_buffer = {};

 public:
  // Thread-safe.
  MovedMousePosition m_mouse_position;                  // Last mouse position, also updated by mouse movement.
  WheelOffset m_accumulated_wheel_offset;               // Accumulated mouse wheel events.

 public:
  void register_input_event_buffer(InputEventBuffer* input_event_buffer)
  {
    m_input_event_buffer = input_event_buffer;
  }

 private:
  // Called when the close button is clicked.
  void On_WM_DELETE_WINDOW(uint32_t timestamp) override
  {
    DoutEntering(dc::notice, "SynchronousWindow::On_WM_DELETE_WINDOW(" << timestamp << ") [" << this << "]");
    // Lets not pass more events to a window that is going to destruct itself.
    // That is, any XCB events received after this WM_DELETE_WINDOW message will be ignored.
    m_input_event_buffer = nullptr;
    // Set the must_close_bit.
    set_must_close();
    // We should have one boost::intrusive_ptr's left: the one in Application::m_window_list.
    // This will be removed from the running task (finish_impl), which prevents the
    // immediate destruction until the task fully finished. At that point this Window
    // will be destructed causing a call to Window::destroy.
  }

  void on_window_size_changed(uint32_t width, uint32_t height) override final;

  void on_map_changed(bool minimized) override final
  {
    DoutEntering(dc::notice, "SynchronousWindow::on_map_changed(" << std::boolalpha << minimized << ")");
    if (minimized)
      set_unmapped();
    else
      set_mapped();
  }

  void on_mouse_move(int16_t x, int16_t y, uint16_t CWDEBUG_ONLY(converted_modifiers)) override final
  {
    DoutEntering(dc::xcbmotion, "vulkan::WindowEvents::on_mouse_move(" << x << ", " << y << ", " << vulkan::ModifierMask{converted_modifiers} << ")");
    MovedMousePosition::wat(m_mouse_position)->set(x, y);
  }

  void on_key_event(int16_t x, int16_t y, uint16_t converted_modifiers, bool pressed, uint32_t keysym) override final
  {
    vulkan::ModifierMask modifiers{converted_modifiers};
    DoutEntering(dc::xcb, "vulkan::WindowEvents::on_key_event(" << x << ", " << y << ", " << modifiers << ", " << std::boolalpha << pressed << ", " << std::hex << keysym << ")");

    if (m_input_event_buffer)
    {
      // Queue event.
      InputEvent event{
        .mouse_position = { x, y },
        .modifier_mask = modifiers,
        .flags = { m_mouse_buttons, pressed ? EventType::key_press : EventType::key_release },
        .keysym = keysym
      };
      if (!m_input_event_buffer->push(&event))
        Dout(dc::warning, "Dumping input event because queue is full!");
    }
  }

  void on_mouse_click(int16_t x, int16_t y, uint16_t converted_modifiers, bool pressed, uint8_t button) override final
  {
    vulkan::ModifierMask modifiers{converted_modifiers};
    // Do not print mouse wheel events when xcb is off. Too noisy.
    DoutEntering(dc::io(!MouseButtons::is_wheel(button))|dc::xcb, "vulkan::WindowEvents::on_mouse_click(" << x << ", " << y <<
        ", " << modifiers << ", " << std::boolalpha << pressed << ", " << (int)button << ")");

    if (MouseButtons::is_wheel(button))
    {
      // The mouse wheel buttons are not handled as separate events (with an order and mouse coordinates),
      // but merely accumulated. This means that the bits for them in m_mouse_buttons are always unset!
      WheelOffset::wat wheel_offset_w(m_accumulated_wheel_offset);
      switch (button)
      {
        // The reasoning for the direction is that I see Left as going to the left on the screen (obviously) which is the negative x direction.
        // Therefore I chose Up as going up on the screen (which is debatable) and that means going in the negative y direction.
        case MouseButtons::WheelUp:
          wheel_offset_w->accumulate_y(-0.5f);
          break;
        case MouseButtons::WheelDown:
          wheel_offset_w->accumulate_y(0.5f);
          break;
        case MouseButtons::WheelLeft:
          wheel_offset_w->accumulate_x(-1.f);
          break;
        case MouseButtons::WheelRight:
          wheel_offset_w->accumulate_x(1.f);
          break;
      }
      return;
    }

    // Cache the last state of the modifiers and button.
    m_mouse_buttons.update_button(button, pressed);

    // Queue event.
    if (m_input_event_buffer)
    {
      // Queue event.
      InputEvent event{
        .mouse_position = { x, y },
        .modifier_mask = modifiers,
        .flags = { m_mouse_buttons, pressed ? EventType::button_press : EventType::button_release },
        .button = button
      };
      if (!m_input_event_buffer->push(&event))
        Dout(dc::warning, "Dumping input event because queue is full!");
    }
  }

  void on_mouse_enter(int16_t x, int16_t y, uint16_t converted_modifiers, bool entered) override final
  {
    vulkan::ModifierMask modifiers{converted_modifiers};
    DoutEntering(dc::notice, "vulkan::WindowEvents::on_mouse_enter(" << x << ", " << y << ", " << modifiers << ", " << std::boolalpha << entered << ")");

    // Queue event.
    if (m_input_event_buffer)
    {
      // Queue event.
      InputEvent event{
        .mouse_position = { x, y },
        .modifier_mask = modifiers,
        .flags = { m_mouse_buttons, entered ? EventType::window_enter : EventType::window_leave },
      };
      if (!m_input_event_buffer->push(&event))
        Dout(dc::warning, "Dumping input event because queue is full!");
    }
  }

  virtual void on_focus_changed(bool in_focus) override final
  {
    DoutEntering(dc::notice, "vulkan::WindowEvents::on_focus_changed(" << std::boolalpha << in_focus << ")");

    // Queue event.
    if (m_input_event_buffer)
    {
      // Queue event.
      InputEvent event{
        .flags = { m_mouse_buttons, in_focus ? EventType::window_in_focus : EventType::window_out_focus },
      };
      if (!m_input_event_buffer->push(&event))
        Dout(dc::warning, "Dumping input event because queue is full!");
    }
  }

 public:
  WindowEvents() = default;

  // The following functions are thread-safe (use a mutex and/or atomics).

  // Used to set the initial (desired) extent.
  // Also used by on_window_size_changed to set the new extent.
  void set_extent(vk::Extent2D const& extent)
  {
    // Take the lock on m_extent.
    linuxviewer::OS::WindowExtent::wat extent_w(m_extent);

    // Do nothing if it wasn't really a change.
    if (extent_w->m_extent == extent)
      return;

    // Change it to the new value.
    extent_w->m_extent = extent;
    set_extent_changed();

    // The change of m_flags will (eventually) be picked up by a call to atomic_flags() (in combination with extent_changed()).
    // That thread then calls get_extent() to read the value that was *last* written to m_extent.

    // Unlock m_extent.
  }
};

} // namespace vulkan
