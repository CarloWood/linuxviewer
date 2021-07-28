#pragma once

#include "GUI_glfw3/gui_Window.h"
#include "utils/InsertExtraInitialization.h"

class Window : public gui::Window
{
 private:
  vk::SurfaceKHR m_surface;

 public:
  using gui::Window::Window;
  ~Window();

  VkExtent2D extent() const
  {
    auto extent = get_glfw_window().getSize();
    return { static_cast<uint32_t>(std::get<0>(extent)), static_cast<uint32_t>(std::get<1>(extent)) };
  }

  // Accessor.
  vk::SurfaceKHR surface() const { return m_surface; }

 private:
  void create_surface();

  // When constructing a Window.
  INSERT_EXTRA_INITIALIZATION { create_surface(); };
};
