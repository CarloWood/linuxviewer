#pragma once

#include "glfwpp/window.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

namespace glfw {
class Monitor;
class Window;
} // namespace glfw

namespace glfw3 {
namespace gui {

struct WindowCreateInfoExt
{
  int const width = 800;                                // The width of the inner window, in pixels.
  int const height = 600;                               // The height of the inner window, in pixels.
  char const* const title = nullptr;                    // The window title; nullptr will cause the application name to be used.
  glfw::Monitor const* const monitor = nullptr;
  glfw::Window const* const share = nullptr;

#ifdef CWDEBUG
  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};

struct WindowCreateInfo : public glfw::WindowHints, public WindowCreateInfoExt
{
};

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;
