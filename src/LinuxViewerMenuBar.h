#pragma once

// We use the GUI implementation on top of glfw3.
#include "GUI_glfw3/gui_MenuEntryKey.h"
//#include "GUI_gtkmm3/gui_MenuEntryKey.h"
using MenuEntryKey = gui::MenuEntryKey;

#include <functional>

// Interface class.
//
// gui::MenuBar derives from this class.

class LinuxViewerMenuBar
{
 public:
  // Request to add a menu entry and use callback function cb for it.
  virtual void append_menu_entry(MenuEntryKey menu_entry_key, std::function<void ()> cb) = 0;
};
