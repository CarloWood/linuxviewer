#include "sys.h"

#ifdef CWDEBUG
#include "gui_WindowCreateInfo.h"
#include <iostream>

namespace glfw3 {
namespace gui {

void WindowCreateInfoExt::print_on(std::ostream& os) const
{
  os << '{';
  os << "width:" << width << ", ";
  os << "height:" << height << ", ";
  os << "title:\"" << title << "\", ";
  os << "monitor:" << monitor << ", ";
  os << "share:\"" << share << "\"";
  os << '}';
}

} // namespace gui
} // namespace glfw3
#endif
