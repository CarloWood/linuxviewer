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
#include <vulkan/vulkan.hpp>
#include <cstring>
#include <iostream>

namespace task {
class SynchronousWindow;
} // namespace vulkan

namespace vulkan {
class SpecialCircumstances;
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
 public:
  ~Window();

  // Called from task state SynchronousWindow_create (initialization).
  void set_xcb_connection(boost::intrusive_ptr<xcb::Connection> xcb_connection) { m_parameters.m_xcb_connection = std::move(xcb_connection); }

  // Create an X Window on the screen and return a vulkan surface handle for it.
  // Called from task state SynchronousWindow_create (initialization).
  [[nodiscard]] vk::UniqueSurfaceKHR create(vk::Instance vh_instance, std::string_view const& title, vk::Extent2D extent);

  // Close the X Window on the screen and decrease the reference count of the xcb connection.
  void destroy();

  // Accessors.
//  WindowParameters const& get_parameters() const { return m_parameters; }

  WindowExtent const& locked_extent() const
  {
    return m_extent;
  }
};

} // namespace linuxviewer::OS
