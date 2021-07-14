#pragma once

#include "GUI_glfw3/gui_WindowCreateInfo.h"

struct WindowCreateInfo : public gui::WindowCreateInfo
{
#ifdef CWDEBUG
  // Printing of this structure.
  void print_on(std::ostream& os) const;
#endif
};
