#pragma once

#include "OperatingSystem.h"
#include "SpecialCircumstances.h"

namespace vulkan {

// Gives asynchronous access to SpecialCircumstances.
class AsyncAccessSpecialCircumstances
{
 private:
  SpecialCircumstances* m_special_circumstances = nullptr;      // Final value is set with set_special_circumstances.

 protected:
  void set_extent_changed() const { m_special_circumstances->m_flags.fetch_or(SpecialCircumstances::extent_changed_bit, std::memory_order::relaxed); }
  void set_must_close() const { m_special_circumstances->m_flags.store(SpecialCircumstances::must_close_bit, std::memory_order::relaxed); }
  void set_have_synchronous_task() const { m_special_circumstances->m_flags.fetch_or(SpecialCircumstances::have_synchronous_task_bit, std::memory_order::relaxed); }
  void set_mapped() const { m_special_circumstances->set_mapped(); }
  void set_unmapped() const { m_special_circumstances->set_unmapped(); }

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
  // Called when the close button is clicked.
  void On_WM_DELETE_WINDOW(uint32_t timestamp) override
  {
    DoutEntering(dc::notice, "SynchronousWindow::On_WM_DELETE_WINDOW(" << timestamp << ")");
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

 public:
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
