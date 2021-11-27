#pragma once

#include <exception>

namespace vulkan {

// Thrown when Device::acquireNextImageKHR returns Result::eErrorOutOfDateKHR.
struct OutOfDateKHR_Exception : public std::exception
{
};

} // namespace vulkan
