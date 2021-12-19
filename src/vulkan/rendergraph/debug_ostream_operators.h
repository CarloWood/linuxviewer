#pragma once

#include <iostream>
#include <vector>

namespace vulkan::rendergraph {

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

} // namespace vulkan::rendergraph
