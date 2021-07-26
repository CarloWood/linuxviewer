#pragma once

#include <vulkan/vulkan.hpp>    // Must be included BEFORE glfwpp/window.h in order to get vulkan C++ API support.
#include <glfwpp/window.h>
#include <iosfwd>

namespace glfw3 {
namespace gui {

std::ostream& operator<<(std::ostream& os, glfw::WindowHints const& window_hints);

} // namespace glfw3
} // namespace gui

namespace gui = glfw3::gui;
