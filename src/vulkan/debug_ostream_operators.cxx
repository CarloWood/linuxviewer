#include "sys.h"
#include "debug_ostream_operators.h"
#include <iostream>

namespace vk {

std::ostream& operator<<(std::ostream& os, Extent2D const& extend_2D)
{
  os << '{';
  os << "width:" << extend_2D.width << ", ";
  os << "height:" << extend_2D.height;
  os << '}';
  return os;
}

std::ostream& operator<<(std::ostream& os, Extent3D const& extend_3D)
{
  os << '{';
  os << "width:" << extend_3D.width << ", ";
  os << "height:" << extend_3D.height << ", ";
  os << "depth:" << extend_3D.depth;
  os << '}';
  return os;
}

} // namespace vk
