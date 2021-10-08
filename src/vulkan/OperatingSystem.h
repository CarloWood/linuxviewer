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
 private:
  WindowParameters m_parameters;
  boost::intrusive_ptr<task::VulkanWindow> m_window_task;

 public:
  Window() = default;
  ~Window() { destroy(); }

  void set_xcb_connection(boost::intrusive_ptr<xcb::Connection> xcb_connection) { m_parameters.m_xcb_connection = std::move(xcb_connection); }
  [[nodiscard]] vk::UniqueSurfaceKHR create(vk::Instance vh_instance, std::string_view const& title, int width, int height, boost::intrusive_ptr<task::VulkanWindow> window_task);
  void destroy();

  WindowParameters get_parameters() const { return m_parameters; }

  void On_WM_DELETE_WINDOW(uint32_t timestamp) override;
  virtual threadpool::Timer::Interval get_frame_rate_interval() const;
};

} // namespace linuxviewer::OS
