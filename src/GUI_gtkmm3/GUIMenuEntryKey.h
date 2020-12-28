#pragma once

#include "LinuxViewerMenuEntries.h"     // menu_keys
#include <gtkmm/stockid.h>              // Gtk::StockID
#ifdef CWDEBUG
#include <iostream>
#endif

namespace gtkmm3 {

struct GUIMenuEntryKey
{
  using TopEntries = menu_keys::TopEntries;
  using MenuEntryWithIconId = menu_keys::MenuEntryWithIconId;
  using MenuEntryWithoutIconId = menu_keys::MenuEntryWithoutIconId;

  TopEntries m_top_entry;
  enum Type { is_stock_id_n, is_menu_entry_without_icon_id_n } m_type;
  Gtk::StockID m_stock_id;                                            // Valid iff m_type == is_stock_id_n.
  MenuEntryWithoutIconId m_menu_entry_without_icon_id;                // Valid iff m_type == is_menu_entry_without_icon_id_n.

  GUIMenuEntryKey(TopEntries top_entry, Gtk::StockID stock_id)
    : m_top_entry(top_entry), m_type(is_stock_id_n), m_stock_id(stock_id) { }

  GUIMenuEntryKey(TopEntries top_entry, MenuEntryWithoutIconId menu_entry_without_icon_id)
    : m_top_entry(top_entry), m_type(is_menu_entry_without_icon_id_n), m_menu_entry_without_icon_id(menu_entry_without_icon_id) { }

  bool is_stock_id() const
  {
    return m_type == is_stock_id_n;
  }

  bool is_menu_entry_without_icon_id() const
  {
    return m_type == is_menu_entry_without_icon_id_n;
  }

  Gtk::StockID get_stock_id() const { return m_stock_id; }
  MenuEntryWithoutIconId get_menu_entry_without_icon_id() const { return m_menu_entry_without_icon_id; }

  friend bool operator<(GUIMenuEntryKey const& key1, GUIMenuEntryKey const& key2)
  {
    return key1.m_top_entry < key2.m_top_entry ||
      (key1.m_top_entry == key2.m_top_entry &&
       ((key1.is_stock_id() && key2.is_menu_entry_without_icon_id()) ||
       ((key1.is_stock_id() == key2.is_stock_id() &&
        ((key1.is_stock_id() &&
          key1.get_stock_id() < key2.get_stock_id()) ||
         (key1.is_menu_entry_without_icon_id() &&
          key1.get_menu_entry_without_icon_id() < key2.get_menu_entry_without_icon_id()))))));
  }

#ifdef CWDEBUG
  friend std::ostream& operator<<(std::ostream& os, GUIMenuEntryKey const& menu_entry_key)
  {
    os << "{top_entry:" << menu_entry_key.m_top_entry << ", menu_entry:";
    if (menu_entry_key.is_stock_id())
      os << menu_entry_key.get_stock_id();
    else if (menu_entry_key.is_menu_entry_without_icon_id())
      os << menu_keys::get_label(menu_entry_key.get_menu_entry_without_icon_id());
    else
      os << "CORRUPT GUIMenuEntryKey";
    return os << "}";
  }
#endif
};

} // namespace gtkmm3
