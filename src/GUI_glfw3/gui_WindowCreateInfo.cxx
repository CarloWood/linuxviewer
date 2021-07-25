#include "sys.h"

#ifdef CWDEBUG
#include "gui_WindowCreateInfo.h"
#include "debug_ostream_operators.h"
#include <iostream>
#include "debug.h"

namespace glfw3 {
namespace gui {

void WindowCreateInfo::print_on(std::ostream& os) const
{
  using NAMESPACE_DEBUG::print_string;

  os << '{';
  os << "hints:" << static_cast<glfw::WindowHints const&>(*this) << ", ";
  os << "width:" << m_width << ", ";
  os << "height:" << m_height << ", ";
  os << "title:" << print_string(m_title) << ", ";
  os << "monitor:" << m_monitor << ", ";
  os << "share:" << m_share << "";
  os << '}';
}

} // namespace gui
} // namespace glfw3
#endif
