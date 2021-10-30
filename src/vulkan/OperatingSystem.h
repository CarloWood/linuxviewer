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
  xcb_window_t          Handle;

  vk::XcbSurfaceCreateInfoKHR get_surface_create_info() const
  {
    return vk::XcbSurfaceCreateInfoKHR{}.setConnection(*m_xcb_connection).setWindow(Handle);
  }
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
  Display*              DisplayPtr;
  Window                Handle;
#endif
};

// ************************************************************ //
// Window                                                       //
//                                                              //
// OS dependent window creation and destruction class           //
// ************************************************************ //
class Window : public xcb::WindowBase
{
 protected:
  WindowParameters m_parameters;                // Window system dependent parameters, like window handle and the connection to the X server.
  vk::Extent2D m_extent;                        // The size of the window.
                                                // Must be updated with m_extent.setWidth(width).setHeight(height); from
                                                // overridden OnWindowSizeChanged(uint32_t width, uint32_t height)
  std::atomic_bool m_must_close = false;        // Is set to true when the window must close.

 public:
  Window() = default;
  ~Window();

  // Called from task state VulkanWindow_create.
  void set_xcb_connection(boost::intrusive_ptr<xcb::Connection> xcb_connection) { m_parameters.m_xcb_connection = std::move(xcb_connection); }

  // Create an X Window on the screen and return a vulkan surface handle for it.
  // Called from task state VulkanWindow_create.
  [[nodiscard]] vk::UniqueSurfaceKHR create(vk::Instance vh_instance, std::string_view const& title, int width, int height);
  // Close the X Window on the screen and decrease the reference count of the xcb connection.
  void destroy();

  // Called when the close button is clicked.
  void On_WM_DELETE_WINDOW(uint32_t timestamp) override
  {
    DoutEntering(dc::notice, "Window::On_WM_DELETE_WINDOW(" << timestamp << ")");
    m_must_close = true;
    // We should have one boost::intrusive_ptr's left: the one in Application::m_window_list.
    // This will be removed from the running task (finish_impl), which prevents the
    // immediate destruction until the task fully finished. At that point this Window
    // will be destructed causing a call to Window::destroy.
  }

  // Used to set the initial (desired) extent.
  void set_extent(vk::Extent2D const& extent) { m_extent = extent; }

  // Accessors.
  WindowParameters get_parameters() const { return m_parameters; }
  vk::Extent2D const& extent() const { return m_extent; }
};

} // namespace linuxviewer::OS
