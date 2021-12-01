#pragma once

#include <exception>

namespace vulkan {

// Thrown when Device::acquireNextImageKHR returns Result::eErrorOutOfDateKHR.
struct OutOfDateKHR_Exception : public std::exception
{
  char const* what() const noexcept override final
  {
    return "eErrorOutOfDateKHR";
  }
};

} // namespace vulkan
