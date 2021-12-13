#pragma once

#include "debug.h"

namespace vulkan {

// Print objects from namespace vulkan, using ADL, if they have a print_on member.
template<typename T>
std::enable_if_t<utils::has_print_on<T const>, std::ostream&>
operator<<(std::ostream& os, T const& data)
{
  data.print_on(os);
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& v)
{
  os << '{';
  char const* prefix = "";
  for (auto&& element : v)
  {
    os << prefix << element;
    prefix = ", ";
  }
  return os << '}';
}

} // namespace vulkan
