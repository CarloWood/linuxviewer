#pragma once

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include "xcb-task/Connection.h"
#include "xcb-task/WindowBase.h"
#include <xcb/xcb.h>
#include <dlfcn.h>
#include <cstdlib>
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <cstdlib>
#endif

#include "threadpool/Timer.h"
#include <cstring>
#include <iostream>

namespace task {
class VulkanWindow;
} // namespace vulkan

namespace linuxviewer::OS {

// ************************************************************ //
// WindowParameters                                             //
//                                                              //
// OS dependent window parameters                               //
// ************************************************************ //
struct WindowParameters
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
  boost::intrusive_ptr<xcb::Connection> m_xcb_connection;
  xcb_window_t          m_handle;

  vk::XcbSurfaceCreateInfoKHR get_surface_create_info() const
  {
    return vk::XcbSurfaceCreateInfoKHR{}.setConnection(*m_xcb_connection).setWindow(m_handle);
  }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  Display*              m_display;
  Window                m_handle;
#endif
};

struct UnlockedWindowExtent
{
  vk::Extent2D m_extent;        // The current extent of the window.
};

using WindowExtent = aithreadsafe::Wrapper<UnlockedWindowExtent, aithreadsafe::policy::Primitive<std::mutex>>;

// ************************************************************ //
// Window                                                       //
//                                                              //
// OS dependent window creation and destruction class           //
// ************************************************************ //
class Window : public xcb::WindowBase
{
 protected:
  WindowParameters m_parameters;                // Window system dependent parameters, like window handle and the connection to the X server.
  WindowExtent m_extent;                        // The size of the window.
                                                // Must be updated with set_extent({ width, height}); from overridden on_window_size_changed(uint32_t width, uint32_t height).

  static constexpr int extent_changed_bit = 1;  // Set after m_extent was changed, so signal the render_loop that the window was resized.
  static constexpr int must_close_bit = 2;      // Set when the window must close.
  static constexpr int cant_render_bit = 4;     // Set when the swapchain is being rebuilt.
  mutable std::atomic_int m_flags;              // Bit mask of the above bits.

 public:
  static bool extent_changed(int flags) { return flags & extent_changed_bit; }
  static bool must_close(int flags) { return flags & must_close_bit; }
  static bool can_render(int flags) { return !(flags & cant_render_bit); }

 public:
  Window() = default;
  ~Window();

  // Called from task state VulkanWindow_create.
  void set_xcb_connection(boost::intrusive_ptr<xcb::Connection> xcb_connection) { m_parameters.m_xcb_connection = std::move(xcb_connection); }

  // Create an X Window on the screen and return a vulkan surface handle for it.
  // Called from task state VulkanWindow_create.
  [[nodiscard]] vk::UniqueSurfaceKHR create(vk::Instance vh_instance, std::string_view const& title, vk::Extent2D extent);
  // Close the X Window on the screen and decrease the reference count of the xcb connection.
  void destroy();

  // Called when the close button is clicked.
  void On_WM_DELETE_WINDOW(uint32_t timestamp) override
  {
    DoutEntering(dc::notice, "Window::On_WM_DELETE_WINDOW(" << timestamp << ")");
    // Set the must_close_bit.
    m_flags.store(must_close_bit, std::memory_order::relaxed);
    // We should have one boost::intrusive_ptr's left: the one in Application::m_window_list.
    // This will be removed from the running task (finish_impl), which prevents the
    // immediate destruction until the task fully finished. At that point this Window
    // will be destructed causing a call to Window::destroy.
  }

  // Used to set the initial (desired) extent.
  // Also used by on_window_size_changed to set the new extent.
  void set_extent(vk::Extent2D const& extent)
  {
    // Take the lock on m_extent.
    WindowExtent::wat extent_w(m_extent);

    // Do nothing if it wasn't really a change.
    if (extent_w->m_extent == extent)
      return;

    // Change it to the new value.
    extent_w->m_extent = extent;
    // Set the extent_changed_bit in m_flags.
    m_flags.fetch_or(extent_changed_bit, std::memory_order::relaxed);
    // The change of m_flags will (eventually) be picked up by a call to atomic_flags() (in combination with extent_changed()).
    // That thread then calls get_extent() to read the value that was *last* written to m_extent.
    //
    // Unlock m_extent.
  }

  // Accessors.
  WindowParameters get_parameters() const { return m_parameters; }
  int atomic_flags() const { return m_flags.load(std::memory_order::relaxed); }

  // Call this from the render loop every time that extent_changed(atomic_flags()) returns true.
  vk::Extent2D get_extent() const
  {
    // Take the lock on m_extent.
    WindowExtent::crat extent_r(m_extent);
    // Read the value that was last written.
    vk::Extent2D extent = extent_r->m_extent;
    // Reset the extent_changed_bit in m_flags.
    m_flags.fetch_and(~extent_changed_bit, std::memory_order::relaxed);
    // Return the new extent and unlock m_extent.
    return extent;
    // Because atomic_flags() is called without taking the lock on m_extent
    // it is theorectically possible (this will NEVER happen in practise)
    // that even after returning here, atomic_flags() will again return
    // extent_changed_bit (even though we just reset it); that then would
    // cause another call to this function reading the same value of the extent.
  }

  // Called by the swapchain.
  void no_can_render() const { m_flags.fetch_or(cant_render_bit, std::memory_order::relaxed); }
  void can_render_again() const { m_flags.fetch_and(~cant_render_bit, std::memory_order::relaxed); }
};

} // namespace linuxviewer::OS
