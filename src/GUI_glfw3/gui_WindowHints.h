#pragma once

#include <vulkan/vulkan.hpp>            // Must be included BEFORE glfwpp/window.h in order to get vulkan C++ API support.
#include <glfwpp/window.h>

namespace glfw3 {
namespace gui {

struct WindowHints : glfw::WindowHints
{
  WindowHints() : glfw::WindowHints{
        .resizable = false,
        //bool floating = false;
        .centerCursor = false,
        //bool transparentFramebuffer = false;
        //bool focusOnShow = true;
        .clientApi = glfw::ClientApi::None
  } { }

  WindowHints& setResizable(bool resizable_)
  {
    resizable = resizable_;
    return *this;
  }

  WindowHints& setFocused(bool focused_)
  {
    focused = focused_;
    return *this;
  }
};

} // namespace gui
} // namespace glfw3

namespace gui = glfw3::gui;
