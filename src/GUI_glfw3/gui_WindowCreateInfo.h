#pragma once

#include <vulkan/vulkan.hpp>    // Must be included BEFORE glfwpp/window.h in order to get vulkan C++ API support.
#include <glfwpp/window.h>
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace glfw {
class Monitor;
class Window;
} // namespace glfw

namespace glfw3 {
namespace gui {

class Window;

struct WindowCreateInfo : public glfw::WindowHints
{
 private:
  friend class Window;                       // Needs read access.
  int m_width                    = 800;      // The width of the inner window, in pixels.
  int m_height                   = 600;      // The height of the inner window, in pixels.
  char const* m_title            = nullptr;  // The window title; nullptr will cause the application name to be used.
  glfw::Monitor const* m_monitor = nullptr;
  glfw::Window const* m_share    = nullptr;

 public:
  WindowCreateInfo(glfw::WindowHints&& window_hints) : glfw::WindowHints(window_hints) {}

  // Setters.
  WindowCreateInfo& setExtent(int width, int height)
  {
    m_width  = width;
    m_height = height;
    return *this;
  }

  WindowCreateInfo& setTitle(char const* title)
  {
    m_title = title;
    return *this;
  }

  WindowCreateInfo& setMonitor(glfw::Monitor const* monitor)
  {
    m_monitor = monitor;
    return *this;
  }

  WindowCreateInfo& setShare(glfw::Window const* share)
  {
    m_share = share;
    return *this;
  }

#ifdef CWDEBUG
  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;
