#pragma once

#include "vk_utils/Badge.h"

namespace task {
class SynchronousTask;
} // namespace task

namespace vulkan {

class AsyncAccessSpecialCircumstances;
class SynchronousEngine;
class Swapchain;

// A wrapper around an atomic int bitmask.
// Several of the bits can be set asynchronically, though all bits are only reset synchronously.
// Also the probe functions may only be called synchronously, giving rise to returning WasFalse
// (can be set async at any time) or True (can only reset synchronously).
//
// The exception here is the cant_render_bit: also can_render_again() is only called
// synchronously; hence that can_render() returns False or True, not a fuzzy boolean.
class SpecialCircumstances
{
 protected:
  static constexpr int extent_changed_bit = 1;  // Set after m_extent was changed, so signal the render_loop that the window was resized.
  static constexpr int must_close_bit = 2;      // Set when the window must close.
  static constexpr int cant_render_bit = 4;     // Set when the swapchain is being rebuilt.
  static constexpr int have_synchronous_task_bit = 8;     // Set when there is a task to perform that can not run asynchronously (while draw_frame is being called).

 private:
  friend class AsyncAccessSpecialCircumstances;
  mutable std::atomic_int m_flags;              // Bit mask of the above bits.

 protected:
  // The fuzzy meanings are for synchronous calls. Non-synchronous calls make no sense.
  static bool extent_changed(int flags) { return flags & extent_changed_bit; }                  // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool must_close(int flags) { return flags & must_close_bit; }                          // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool can_render(int flags) { return !(flags & cant_render_bit); }                      // true = fuzzy::WasTrue, false = fuzzy::False.
  static bool have_synchronous_task(int flags) { return flags & have_synchronous_task_bit; }    // true = fuzzy::True, false = fuzzy::WasFalse.

  // Accessor. The result must be passed to one of the above decoders.
  int atomic_flags() const { return m_flags.load(std::memory_order::relaxed); }

  // Control the extent_changed_bit.
  void reset_extent_changed() const { m_flags.fetch_and(~extent_changed_bit, std::memory_order::relaxed); }
  // Called when vk::Result::eErrorOutOfDateKHR happens in SynchronousWindow::acquire_image or SynchronousWindow::finish_frame.
  void set_extent_changed() const { m_flags.fetch_or(extent_changed_bit, std::memory_order::relaxed); }

  // Called when the swapchain is being (re)created. Called from Swapchain::prepare and Swapchain::recreate.
  void no_can_render() const { m_flags.fetch_or(cant_render_bit, std::memory_order::relaxed); }
  // Called when the swapchain creation finished and we can render again. Called from 
  void can_render_again() const { m_flags.fetch_and(~cant_render_bit, std::memory_order::relaxed); }

  // Control the have_synchronous_task_bit.
  void reset_have_synchronous_task(vk_utils::Badge<SynchronousEngine>) const { m_flags.fetch_and(~have_synchronous_task_bit, std::memory_order::relaxed); }
  void set_have_synchronous_task(vk_utils::Badge<
      task::SynchronousTask,    // When running a SynchronousTask (calling SynchronousTask::run).
      SynchronousEngine         // When active tasks remained after the mainloop returned.
      >) const { m_flags.fetch_or(have_synchronous_task_bit, std::memory_order::relaxed); }
};

} // namespace vulkan
