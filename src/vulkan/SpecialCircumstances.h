#pragma once

#include "utils/Badge.h"
#include <atomic>

namespace vulkan {

namespace task {
class SynchronousTask;
} // namespace task

class AsyncAccessSpecialCircumstances;
class SynchronousEngine;
class Swapchain;

// A wrapper around an atomic int bitmask.
// Several of the bits can be set asynchronically, though all bits are only reset synchronously.
// Also the probe functions may only be called synchronously, giving rise to returning WasFalse
// (can be set async at any time) or True (can only reset synchronously).
//
// The exception here are have_no_swapchain_bit and minimized_bit: also can_render_again() is only
// called synchronously; hence that can_render() returns False or True, not a fuzzy boolean.
class SpecialCircumstances
{
 protected:
  static constexpr int extent_changed_bit = 1;          // Set after m_extent was changed, so signal the render_loop that the window was resized.
  static constexpr int must_close_bit = 2;              // Set when the window must close.
  static constexpr int have_no_swapchain_bit = 4;       // Set when the swapchain is being rebuilt.
  static constexpr int have_synchronous_task_bit = 8;   // Set when there is a task to perform that can not run asynchronously (while render_frame is being called).
  static constexpr int minimized_bit = 16;              // Set when the render loop doesn't want to draw because it assumes the window is minimized.
  static constexpr int map_changed_bit = 32;            // Set when a the window was just (un)minimized.

  //
  // Four states A, B, C and D:
  //
  //         __s__
  //         \   /
  //          v /     a: a-synchronous thread.
  //          (B)     s: synchronous thread.
  //          /^^
  //         s | a
  //        v  |  \
  //       (A) a (C)
  //        \  |  ^
  //         a | s
  //          vv/
  //          (D)
  //          ^ \
  //         /_s_\
  //
  // A (handled) and B (unhandled) mean UNMAPPED (minimized).
  // C (handled) and D (unhandled) mean MAPPED.
  //
  // AsyncAccessSpecialCircumstances can change between MAPPED and UNMAPPED,
  // always towards (or staying in) unhandled.
  //
  // SynchronousEngine can change the state from unhandled to handled,
  // never changing the MAPPED/UNMAPPED state.
  //
  // If we choose A=01, B=00, C=10 and D=11:
  //
  static constexpr int handled_UNMAPPED = 1;    // A
  static constexpr int unhandled_UNMAPPED = 0;  // B
  static constexpr int handled_MAPPED = 2;      // C
  static constexpr int unhandled_MAPPED = 3;    // D

  // Then
  //   Async: UNMAPPED -> MAPPED = A -> D = B -> D =  OR with 11.
  //   Async: MAPPED -> UNMAPPED = C -> B = D -> B = AND with 00.
  //   Sync: handle UNMAPPED     = B -> A = D -> D =  OR with 01.
  //   Sync: handle MAPPED       = D -> C = B -> B = AND with 10.
  //
  static constexpr int OR_UNMAPPED_to_MAPPED = 3;
  static constexpr int AND_MAPPED_to_UNMAPPED = 0;
  static constexpr int OR_handle_UNMAPPED = 1;
  static constexpr int AND_handle_MAPPED = 2;

  // It's logic.
  static_assert((  handled_UNMAPPED | OR_UNMAPPED_to_MAPPED)  == unhandled_MAPPED   &&
                (unhandled_UNMAPPED | OR_UNMAPPED_to_MAPPED)  == unhandled_MAPPED,   "logic fail");
  static_assert((  handled_MAPPED   & AND_MAPPED_to_UNMAPPED) == unhandled_UNMAPPED &&
                (unhandled_MAPPED   & AND_MAPPED_to_UNMAPPED) == unhandled_UNMAPPED, "logic fail");
  static_assert((unhandled_UNMAPPED | OR_handle_UNMAPPED)     == handled_UNMAPPED   &&
                (unhandled_MAPPED   | OR_handle_UNMAPPED)     == unhandled_MAPPED,   "logic fail");
  static_assert((unhandled_MAPPED   & AND_handle_MAPPED)      == handled_MAPPED     &&
                (unhandled_UNMAPPED & AND_handle_MAPPED)      == unhandled_UNMAPPED, "logic fail");

  // It's mapped when this bit is set.
  static constexpr int MAPPED_bit = 2;

 private:
  friend class AsyncAccessSpecialCircumstances;
  mutable std::atomic_int m_flags;              // Bit mask of the above bits.
  mutable std::atomic_int m_map_flags{handled_MAPPED};

  bool is_mapped() const { return (m_map_flags.load(std::memory_order::relaxed) & MAPPED_bit); }

 public:
#ifdef CWDEBUG
  static char const* print_map_flags(int map_flags);
#endif

 protected:
  // The fuzzy meanings are for synchronous calls. Non-synchronous calls make no sense.
  static bool extent_changed(int flags) { return flags & extent_changed_bit; }                  // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool must_close(int flags) { return flags & must_close_bit; }                          // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool can_render(int flags) { return !(flags & (have_no_swapchain_bit|minimized_bit)); }// true = fuzzy::WasTrue, false = fuzzy::False.
  static bool have_synchronous_task(int flags) { return flags & have_synchronous_task_bit; }    // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool map_changed(int flags) { return flags & map_changed_bit; }                        // true = fuzzy::True, false = fuzzy::WasFalse.

  // Accessor. The result must be passed to one of the above decoders.
  int atomic_flags() const { return m_flags.load(std::memory_order::acquire); }

  static bool is_mapped(int map_flags) { return (map_flags & MAPPED_bit); }                     // Fuzzy either way. Races are detected in handled_map_changed.

  // Accessor.
  int atomic_map_flags() const { return m_map_flags.load(std::memory_order::relaxed); }

  // Returns true on success, meaning - the map change is now handled.
  // False means that there is another special circumstance that needs handling.
  bool handled_map_changed(int map_flags);

  // Control the extent_changed_bit.
  void reset_extent_changed() const { m_flags.fetch_and(~extent_changed_bit, std::memory_order::relaxed); }
  // Called when vk::Result::eErrorOutOfDateKHR happens in SynchronousWindow::acquire_image or SynchronousWindow::finish_frame.
  void set_extent_changed() const { m_flags.fetch_or(extent_changed_bit, std::memory_order::relaxed); }

  // Control the must_close_bit bit.
  void set_must_close() const { m_flags.fetch_or(must_close_bit, std::memory_order::relaxed); }

  // Called when the swapchain is being (re)created. Called from Swapchain::prepare and Swapchain::recreate.
  void no_swapchain() const { m_flags.fetch_or(have_no_swapchain_bit, std::memory_order::relaxed); }
  // Called when the swapchain creation finished and we can render again. Called from 
  void have_swapchain() const { m_flags.fetch_and(~have_no_swapchain_bit, std::memory_order::relaxed); }

  // Control the have_synchronous_task_bit.
  void reset_have_synchronous_task(utils::Badge<SynchronousEngine>) const { m_flags.fetch_and(~have_synchronous_task_bit, std::memory_order::relaxed); }
  void set_have_synchronous_task(utils::Badge<
      task::SynchronousTask,    // When running a SynchronousTask (calling SynchronousTask::run).
      SynchronousEngine         // When active tasks remained after the mainloop returned.
      >) const { m_flags.fetch_or(have_synchronous_task_bit, std::memory_order::relaxed); }

  // Control the map_changed_bit.
  // Called when the window was just minimized.
  void set_mapped() const;
  // Called when the window was just unminimized.
  void set_unmapped() const;
  // Called when the render loop picked up map_changed_bit being set.
  void reset_map_changed() const { m_flags.fetch_and(~map_changed_bit, std::memory_order::relaxed); }

  // Control the minimized_bit. Both functions are only called by the SynchronousWindow.
  // minimized_bit doesn't really have to be atomic therefore, but for not it seems handy.
  void set_minimized() const { m_flags.fetch_or(minimized_bit, std::memory_order::relaxed); }
  void set_unminimized() const { m_flags.fetch_and(~minimized_bit, std::memory_order::relaxed); }
};

} // namespace vulkan
