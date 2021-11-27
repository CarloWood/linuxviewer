#pragma once

namespace vulkan {

class SpecialCircumstances
{
 protected:
  static constexpr int extent_changed_bit = 1;  // Set after m_extent was changed, so signal the render_loop that the window was resized.
  static constexpr int must_close_bit = 2;      // Set when the window must close.
  static constexpr int cant_render_bit = 4;     // Set when the swapchain is being rebuilt.
  static constexpr int have_synchronous_task_bit = 8;     // Set when there is a task to perform that can not run asynchronously (while draw_frame is being called).
  mutable std::atomic_int m_flags;              // Bit mask of the above bits.

 public:
  // The fuzzy meanings are for synchronous calls. Non-synchronous calls make no sense.
  static bool extent_changed(int flags) { return flags & extent_changed_bit; }                  // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool must_close(int flags) { return flags & must_close_bit; }                          // true = fuzzy::True, false = fuzzy::WasFalse.
  static bool can_render(int flags) { return !(flags & cant_render_bit); }                      // true = fuzzy::WasTrue, false = fuzzy::False.
  static bool have_synchronous_task(int flags) { return flags & have_synchronous_task_bit; }    // true = fuzzy::True, false = fuzzy::WasFalse.

  // Accessor.
  int atomic_flags() const { return m_flags.load(std::memory_order::relaxed); }

  // Control the extent_changed_bit.
  void set_extent_changed() const { m_flags.fetch_or(extent_changed_bit, std::memory_order::relaxed); }
  void reset_extent_changed() const { m_flags.fetch_and(~extent_changed_bit, std::memory_order::relaxed); }

  // Control the must_close_bit.
  void set_must_close() const { m_flags.store(must_close_bit, std::memory_order::relaxed); }

  // Control the cant_render_bit, called by the swapchain.
  void no_can_render() const { m_flags.fetch_or(cant_render_bit, std::memory_order::relaxed); }
  void can_render_again() const { m_flags.fetch_and(~cant_render_bit, std::memory_order::relaxed); }                            // May only be called synchronously.

  // Control the have_synchronous_task_bit.
  void set_have_synchronous_task() const { m_flags.fetch_or(have_synchronous_task_bit, std::memory_order::relaxed); }
  void reset_have_synchronous_task() const { m_flags.fetch_and(~have_synchronous_task_bit, std::memory_order::relaxed); }       // May only be called synchronously.
};

} // namespace vulkan
