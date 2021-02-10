#pragma once

#include "GUI_glfw3/gui_MenuEntryKey.h"
using MenuEntryKey = glfw3::gui::MenuEntryKey;

// Interface class.
//
// GUIMenuBar derived from this class.

class LinuxViewerMenuBar
{
 public:
  // Request to add a menu entry and use callback function cb for it.
  virtual void append_menu_entry(MenuEntryKey menu_entry_key, std::function<void ()> cb) = 0;
};
