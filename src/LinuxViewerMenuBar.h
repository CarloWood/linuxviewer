#pragma once

#include "GUI_gtkmm3/GUIMenuEntryKey.h"
using GUIMenuEntryKey = gtkmm3::GUIMenuEntryKey;

// Interface class.
//
// GUIMenuBar derived from this class.

class LinuxViewerMenuBar
{
 public:
  // Request to add a menu entry and use callback function cb for it.
  virtual void append_menu_entry(GUIMenuEntryKey menu_entry_key, std::function<void ()> cb) = 0;
};
