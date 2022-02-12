#include "sys.h"
#include "SynchronousWindow.h"
#include "OperatingSystem.h"
#include "LogicalDevice.h"
#include "debug.h"
#include <thread>
#include <chrono>

namespace linuxviewer::OS {

#if defined(VK_USE_PLATFORM_XCB_KHR)

Window::~Window()
{
  destroy();
}

vk::UniqueSurfaceKHR Window::create(vk::Instance vh_instance, std::string_view const& title, vk::Extent2D extent, Window const* parent_window)
{
  DoutEntering(dc::vulkan, "linuxviewer::OS::Window::create(vh_instance, \"" << title << "\", " << extent << ", " << parent_window << ") [" << this << "]");

  m_parameters.m_handle = m_parameters.m_xcb_connection->generate_id();
  Dout(dc::vulkan, "Generated window handle: " << m_parameters.m_handle);

  // Add this handle to the handle->window lookup map of the connection.
  m_parameters.m_xcb_connection->add(m_parameters.m_handle, this);

  std::vector<uint32_t> const value_list = {
    m_parameters.m_xcb_connection->white_pixel(),
    XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_POINTER_MOTION |
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
    XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
    XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY
  };

  xcb_window_t const parent_handle = parent_window ? parent_window->window_parameters().m_handle : xcb_window_t{};

  m_parameters.m_xcb_connection->create_window(
//    XCB_COPY_FROM_PARENT,                     // depth
    m_parameters.m_handle,                      // window handle
    parent_handle,                              // parent window
    20,                                         // x offset
    20,                                         // y offset
    extent.width,                               // width
    extent.height,                              // height
    title,
    0,                                          // border_width
    XCB_WINDOW_CLASS_INPUT_OUTPUT,              // window class
//  screen->root_visual,                        // visual
    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,      // value mask
    value_list);                                // value list

  // Create a vulkan surface for this window.
  auto surface_create_info = m_parameters.get_surface_create_info();
  return vh_instance.createXcbSurfaceKHRUnique(surface_create_info);
}

void Window::destroy()
{
  DoutEntering(dc::vulkan, "linuxviewer::OS::Window::destroy()");

  if (m_parameters.m_xcb_connection)
  {
    m_parameters.m_xcb_connection->destroy_window(m_parameters.m_handle);
    m_parameters.m_xcb_connection.reset();
  }
  // Only now it is safe to destroy the parent window (now that we called `destroy_window`).
  // The reason for that is that when the parent window calls `xcb_destroy_window` that
  // causes a `XCB_DESTROY_NOTIFY` for all of its children which in turn will remove
  // the window handle of that message from the list of known handles. If we'd do a
  // call to `destroy_window` afterwards then we wouldn't find that handle in the
  // list which causes an assert (it is debatable if the assert is correct).

  // By returning from this function the intrusive_ptr to the parent window is
  // destroyed. Or put differently: the child window must destroy this Window
  // before it destroys the intrusive_ptr to the parent window.
}

#elif defined(VK_USE_PLATFORM_XLIB_KHR)

bool Window::create(char const* title, int width, int height)
{
  m_parameters.DisplayPtr = XOpenDisplay(nullptr);
  if (!m_parameters.DisplayPtr)
    return false;

  int default_screen = DefaultScreen(m_parameters.DisplayPtr);

  m_parameters.Handle = XCreateSimpleWindow(m_parameters.DisplayPtr, DefaultRootWindow(m_parameters.DisplayPtr), 20, 20, width, height, 1,
      BlackPixel(m_parameters.DisplayPtr, default_screen), WhitePixel(m_parameters.DisplayPtr, default_screen));

  // XSync(m_parameters.DisplayPtr, false);
  XSetStandardProperties(m_parameters.DisplayPtr, m_parameters.Handle, title, title, None, nullptr, 0, nullptr);
  XSelectInput(m_parameters.DisplayPtr, m_parameters.Handle, ExposureMask | KeyPressMask | StructureNotifyMask);

  return true;
}

Window::~Window()
{
  if (m_parameters.DisplayPtr)
  {
    XDestroyWindow(m_parameters.DisplayPtr, m_parameters.Handle);
    XCloseDisplay(m_parameters.DisplayPtr);
    m_parameters.DisplayPtr = nullptr;
  }
}

bool Window::rendering_loop(ProjectBase& project) const
{
  // Prepare notification for window destruction
  Atom delete_window_atom;
  delete_window_atom = XInternAtom(m_parameters.DisplayPtr, "WM_DELETE_WINDOW", false);
  XSetWMProtocols(m_parameters.DisplayPtr, m_parameters.Handle, &delete_window_atom, 1);

  // Display window
  XClearWindow(m_parameters.DisplayPtr, m_parameters.Handle);
  XMapWindow(m_parameters.DisplayPtr, m_parameters.Handle);

  // Main message loop
  XEvent event;
  bool loop   = true;
  bool resize = false;
  bool result = true;

  while (loop)
  {
    if (XPending(m_parameters.DisplayPtr))
    {
      XNextEvent(m_parameters.DisplayPtr, &event);
      switch (event.type)
      {
          //Process events
        case ConfigureNotify:
        {
          static int width  = event.xconfigure.width;
          static int height = event.xconfigure.height;

          if (((event.xconfigure.width > 0) && (event.xconfigure.width != width)) || ((event.xconfigure.height > 0) && (event.xconfigure.width != height)))
          {
            width  = event.xconfigure.width;
            height = event.xconfigure.height;
            resize = true;
          }
        }
        break;
        case KeyPress:
          loop = false;
          break;
        case DestroyNotify:
          loop = false;
          break;
        case ClientMessage:
          if (static_cast<unsigned int>(event.xclient.data.l[0]) == delete_window_atom)
            loop = false;
          break;
      }
    }
    else
    {
      // Draw
      if (resize)
      {
        resize = false;
        project.on_window_size_changed();
      }
      if (project.ReadyToDraw())
      {
        if (!project.Draw())
        {
          result = false;
          break;
        }
      }
      else
      {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
    }
  }

  return result;
}
#endif

} // namespace linuxviewer::OS
