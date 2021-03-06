#pragma once

#include "LinuxViewerMenuEntries.h"
#include <gtkmm.h>
#include <array>

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {
namespace gui {

class IconFactory
{
 public:
  IconFactory();

  Gtk::StockID get_icon_id(menu_keys::MenuEntryWithIconId menu_entry_id) const { return m_icon_ids[menu_entry_id]; }

 private:
  struct icon_info_st
  {
    char const* filepath;
    char const* id;
    char const* label;
  };

 protected:
  static std::array<icon_info_st, menu_keys::number_of_custom_icons> s_icon_info;
  std::array<Gtk::StockID, menu_keys::number_of_custom_icons> m_icon_ids;
};

} // namespace gui
} // namespace gtkmm3
