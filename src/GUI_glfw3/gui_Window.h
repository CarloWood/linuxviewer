#pragma once

#include <vulkan/vulkan.hpp>            // Must be included BEFORE glfwpp/glfwpp.h in order to get vulkan C++ API support.
#include <glfwpp/glfwpp.h>
#include "gui_WindowCreateInfo.h"

// This is the GUI implementation that is implemented on top of glfw3.
namespace glfw3 {
namespace gui {

class MenuBar;
class Application;

class Window
{
 private:
  Application* m_application;           // The appliction that was passed to the constructor.
  glfw::Window m_window;                // The underlaying window implementation.

 public:
  // The Application must have a life-time that is longer than that of the window:
  // the applications main loop (the run() function) may only return when ALL windows have been destroyed.
  Window(Application* application, WindowCreateInfo const&);

  // Note that glfw::Window does not have a virtual table (or virtual destructor), so don't use a glfw::Window* to delete this object.
  virtual ~Window();

  void append_menu_entries(MenuBar* menubar);

  // Accessor for underlaying window.
  glfw::Window& get_glfw_window() { return m_window; }
  glfw::Window const& get_glfw_window() const { return m_window; }

  VkExtent2D getExtent() const
  {
    auto extent = m_window.getSize();
    return { static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) };
  }

 protected:
  friend class MenuBar;              // Calls append_menu_entries on the two objects returned by the following accessors.
  Application& application() const { return *m_application; }

#if 0
  // Child widgets.
  Gtk::Grid m_grid;
  MenuBar* m_menubar;
#endif
};

} // namespace gui
} // namespace glfw3
