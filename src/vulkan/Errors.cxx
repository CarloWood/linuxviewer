#include "sys.h"
#include "Errors.h"
#include <iostream>

namespace VULKAN_HPP_NAMESPACE {

std::ostream& operator<<(std::ostream& os, Result code)
{
  return os << to_string(code);
}

} // namespace VULKAN_HPP_NAMESPACE
