#include "sys.h"
#include "WindowCreateInfo.h"
#ifdef CWDEBUG
#include <iostream>
#endif

#ifdef CWDEBUG
void WindowCreateInfo::print_on(std::ostream& os) const
{
  os << '{';
  gui::WindowCreateInfo::print_on(os);
  os << '}';
}
#endif
