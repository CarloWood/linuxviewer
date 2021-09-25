#include "sys.h"
#include "VulkanWindow.h"
#include "OperatingSystem.h"
#include "debug.h"
#include <thread>
#include <chrono>

namespace linuxviewer::OS {

#if defined(VK_USE_PLATFORM_XCB_KHR)

void Window::create(std::string_view const& title, int width, int height, boost::intrusive_ptr<task::VulkanWindow> window_task)
{
  DoutEntering(dc::vulkan, "linuxviewer::OS::Window::create(\"" << title << "\", " << width << ", " << height << ") [" << this << "]");

  m_parameters.Handle = m_parameters.m_xcb_connection->generate_id();
  Dout(dc::vulkan, "Generated window handle: " << m_parameters.Handle);

  // Add this handle to the handle->window lookup map of the connection.
  m_parameters.m_xcb_connection->add(m_parameters.Handle, this);

  std::vector<uint32_t> const value_list = {
    m_parameters.m_xcb_connection->white_pixel(),
    XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE
  };

  // Keep a reference to the corresponding window task.
  m_window_task = std::move(window_task);

  // Set the maximum frame rate.
  m_window_task->set_frame_rate_interval(get_frame_rate_interval());

  m_parameters.m_xcb_connection->create_main_window(
//    XCB_COPY_FROM_PARENT,                       // depth
    m_parameters.Handle,                        // window handle
//    screen->root,                               // parent window
    20,                                         // x offset
    20,                                         // y offset
    width,                                      // width
    height,                                     // height
    title,
    0,                                          // border_width
    XCB_WINDOW_CLASS_INPUT_OUTPUT,              // window class
//    screen->root_visual,                        // visual
    XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,      // value mask
    value_list);                                // value list
}

void Window::destroy()
{
  DoutEntering(dc::vulkan, "linuxviewer::OS::Window::destroy()");

  if (m_parameters.m_xcb_connection)
  {
    m_parameters.m_xcb_connection->destroy_window(m_parameters.Handle);
    m_parameters.m_xcb_connection.reset();
  }
}

#if 0
bool Window::rendering_loop(ProjectBase& project) const
{
  // Main message loop
  xcb_generic_event_t* event;
  bool resize = false;

  while (loop)
  {
    event = nullptr; //FIXME xcb_poll_for_event(m_parameters.Connection);

    if (event)
    {
      // Process events
      switch (event->response_type & 0x7f)
      {
        // Mouse button press
      case XCB_BUTTON_PRESS:
      {
        xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
        uint32_t mask = ev->state;
        std::string s = print_modifiers(mask);
        Dout(dc::notice, s);

        switch (ev->detail) {
        case 4:
          Dout(dc::notice, "Wheel Button up in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");
          break;
        case 5:
          Dout(dc::notice, "Wheel Button down in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");
          break;
        default:
          Dout(dc::notice, "Button " << ev->detail << "pressed in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");

#if 0
          project.MouseMove(ev->event_x, ev->event_y);
          if (ev->detail == 1)
            project.MouseClick(0, true);
          else if (ev->detail == 3)
            project.MouseClick(1, true);
#endif
        }
        break;
      }
        // Mouse button release
      case XCB_BUTTON_RELEASE:
      {
        xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
        Dout(dc::notice, print_modifiers(ev->state));
        Dout(dc::notice, "Button " << ev->detail << "released in window " << ev->event << ", at coordinates (" << ev->event_x << ", " << ev->event_y << ")");

#if 0
        project.MouseMove(ev->event_x, ev->event_y);
        if (ev->detail == 1)
          project.MouseClick(0, false);
        else if (ev->detail == 3)
          project.MouseClick(1, false);
#endif
        break;
      }
        // Resize
      case XCB_CONFIGURE_NOTIFY:
      {
        xcb_configure_notify_event_t* configure_event = (xcb_configure_notify_event_t*)event;
        static uint16_t width = configure_event->width;
        static uint16_t height = configure_event->height;

        if (((configure_event->width > 0) && (width != configure_event->width)) ||
          ((configure_event->height > 0) && (height != configure_event->height)))
        {
          resize = true;
          width = configure_event->width;
          height = configure_event->height;
        }
        break;
      }
        // Close
      case XCB_CLIENT_MESSAGE:
#if 0 // FIXME
        if ((*(xcb_client_message_event_t*)event).data.data32[0] == (*delete_reply).atom)
        {
          loop = false;
          free(delete_reply);
        }
#endif
        break;
      case XCB_KEY_PRESS:
        loop = false;
        break;
      }
      free(event);
    }
  }
}
#endif

//virtual
threadpool::Timer::Interval Window::get_frame_rate_interval() const
{
  return threadpool::Interval<10, std::chrono::milliseconds>{};
}

#if 0
bool Window::rendering_loop()
{
  bool result = true;
  bool loop = true;
  bool resize = false;

  while (loop)
  {
    // Draw
    if (resize)
    {
      resize = false;
      OnWindowSizeChanged();
    }
#if 0
    if (ReadyToDraw())
    {
      Draw();
    }
    else
#endif
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  return result;
}
#endif

void Window::On_WM_DELETE_WINDOW(uint32_t timestamp)
{
  DoutEntering(dc::notice, "Window::On_WM_DELETE_WINDOW(" << timestamp << ")");
  // In practise this test should never fail, but since it takes a while before a window
  // is really closed, it is possible that someone clicks on the close button fast enough
  // that two or more WM_DELETE_WINDOW events get through for the same window.
  // This test avoids a crash in that case.
  if (m_window_task)
    m_window_task->close();
  m_window_task.reset();
  // At this point the task MUST still be kept alive, because upon destruction
  // it will destruct this Window and we can't have that while still inside
  // a virtual function.
  //
  // We should have one boost::intrusive_ptr's left: the one in Application::m_window_list.
  // This will be removed from the running task (finish_impl), which prevents the
  // immediate destruction until the task fully finished. At that point this Window
  // will be destructed causing a call to Window::destroy.
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
        project.OnWindowSizeChanged();
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
