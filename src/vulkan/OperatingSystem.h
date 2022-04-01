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

#include "InputEvent.h"
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
  [[nodiscard]] vk::UniqueSurfaceKHR create(vk::Instance vh_instance, std::u8string_view const& title, vk::Rect2D geometry, Window const* parent_window);

  // Close the X Window on the screen and decrease the reference count of the xcb connection.
  void destroy();

  // Accessors.
  WindowParameters const& window_parameters() const { return m_parameters; }

  WindowExtent const& locked_extent() const
  {
    return m_extent;
  }

  uint16_t convert(uint32_t modifiers) override final
  {
    // Expected XCB mapping (see xcb-task/WindowBase.h):
    //
    // static constexpr int Shift   = 1 << 0;
    // static constexpr int Lock    = 1 << 1;
    // static constexpr int Ctrl    = 1 << 2;
    // static constexpr int Alt     = 1 << 3;
    // static constexpr int Super   = 1 << 6;
    static_assert(
        xcb::ModifierMask::Shift == 1 << 0 &&
        xcb::ModifierMask::Lock  == 1 << 1 &&
        xcb::ModifierMask::Ctrl  == 1 << 2 &&
        xcb::ModifierMask::Alt   == 1 << 3 &&
        xcb::ModifierMask::Super == 1 << 6, "You're only paranoid if AREN'T after you.");
    //
    // Required mapping (see InputEvent.h):
    //
    // static constexpr int Ctrl    = 1 << 0;
    // static constexpr int Shift   = 1 << 1;
    // static constexpr int Alt     = 1 << 2;
    // static constexpr int Super   = 1 << 3;
    // static constexpr int Lock    = 1 << 4;
    static_assert(
        vulkan::ModifierMask::Ctrl  == 1 << 0 &&
        vulkan::ModifierMask::Shift == 1 << 1 &&
        vulkan::ModifierMask::Alt   == 1 << 2 &&
        vulkan::ModifierMask::Super == 1 << 3 &&
        vulkan::ModifierMask::Lock  == 1 << 4, "What he said.");

    auto lowkey = modifiers & (xcb::ModifierMask::Shift|xcb::ModifierMask::Lock|xcb::ModifierMask::Ctrl|xcb::ModifierMask::Alt);
    uint16_t superbit = (modifiers & xcb::ModifierMask::Super) >> 3;

    // Create a few aliases for convenience.
    auto&& C = vulkan::ModifierMask::Ctrl;
    auto&& S = vulkan::ModifierMask::Shift;
    auto&& A = vulkan::ModifierMask::Alt;
    auto&& W = vulkan::ModifierMask::Super;
    auto&& L = vulkan::ModifierMask::Lock;

    alignas(config::cacheline_size_c) static std::array<uint16_t, 16> conversion =
    {
      // A C L S <-- xcb bit order
      /* 0 0 0 0 */ 0,
      /* 0 0 0 1 */       S,
      /* 0 0 1 0 */     L,
      /* 0 0 1 1 */     L|S,
      /* 0 1 0 0 */   C,
      /* 0 1 0 1 */   C|  S,
      /* 0 1 1 0 */   C|L,
      /* 0 1 1 1 */   C|L|S,
      /* 1 0 0 0 */ A,
      /* 1 0 0 1 */ A|    S,
      /* 1 0 1 0 */ A|  L,
      /* 1 0 1 1 */ A|  L|S,
      /* 1 1 0 0 */ A|C,
      /* 1 1 0 1 */ A|C|  S,
      /* 1 1 1 0 */ A|C|L,
      /* 1 1 1 1 */ A|C|L|S,
    };
    // Convert xcb::ModifierMask to vulkan::ModifierMask.
    return conversion[lowkey] | superbit;
  }
};

} // namespace linuxviewer::OS
