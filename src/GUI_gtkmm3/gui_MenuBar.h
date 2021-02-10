#pragma once

#include "gui_IconFactory.h"
#include "LinuxViewerMenuBar.h"
#include "debug.h"
#include <gtkmm.h>
#include <array>
#include <map>
#include <any>

// This is the GUI implementation that is implemented on top of gtkmm3.
namespace gtkmm3 {
namespace gui {

class Window;

class MenuBar : public Gtk::MenuBar, public IconFactory, public LinuxViewerMenuBar
{
  using TopEntries = menu_keys::TopEntries;
  using MenuEntryWithIconId = menu_keys::MenuEntryWithIconId;
  using MenuEntryWithoutIconId = menu_keys::MenuEntryWithoutIconId;

 public:
  MenuBar(Window* main_window);

 protected:
  std::array<Gtk::Menu*, menu_keys::number_of_top_entries> m_submenus;
  std::map<MenuEntryKey, Gtk::MenuItem*> m_menu_items;

  Gtk::SeparatorMenuItem* m_separator;

 public:
  void append_menu_entry(MenuEntryKey menu_entry_key, std::function<void ()> cb) override
  {
    Gtk::MenuItem* menu_item_ptr;
    if (menu_entry_key.is_stock_id())
      menu_item_ptr = Gtk::manage(new Gtk::ImageMenuItem(menu_entry_key.get_stock_id()));
    else if (menu_entry_key.is_menu_entry_without_icon_id())
      menu_item_ptr = Gtk::manage(new Gtk::MenuItem(get_label(menu_entry_key.get_menu_entry_without_icon_id())));
    else
      ASSERT(false);
    m_menu_items[menu_entry_key] = menu_item_ptr;
    m_submenus[menu_entry_key.m_top_entry]->append(*menu_item_ptr);
    menu_item_ptr->signal_activate().connect(cb);
  }

  void append_separator(int top_entry)
  {
    Gtk::MenuItem* separator;
    separator = Gtk::manage(new Gtk::SeparatorMenuItem);
    m_submenus[top_entry]->append(*separator);
  }

  struct MenuEntryKeyStub
  {
    TopEntries m_top_entry;
    MenuEntryWithIconId m_menu_entry;
  };

  template<class T>
  void append_menu_entry(MenuEntryKeyStub menu_entry_key, T* obj, void (T::*cb)())
  {
    append_menu_entry({menu_entry_key.m_top_entry, get_icon_id(menu_entry_key.m_menu_entry)}, obj, cb);
  }

  template<class T>
  void append_radio_menu_entry(Gtk::RadioButtonGroup& group, MenuEntryKey menu_entry_key, T* obj, void (T::*cb)())
  {
    ASSERT(menu_entry_key.is_menu_entry_without_icon_id());     // None of our radio menu items have icons.
    Gtk::RadioMenuItem* menu_item_ptr = Gtk::manage(new Gtk::RadioMenuItem(group, get_label(menu_entry_key.get_menu_entry_without_icon_id())));
    m_menu_items[menu_entry_key] = menu_item_ptr;
    m_submenus[menu_entry_key.m_top_entry]->append(*menu_item_ptr);
    menu_item_ptr->signal_activate().connect(sigc::mem_fun(*obj, cb));
  }

  void activate(MenuEntryKey menu_entry_key)
  {
    auto item = m_menu_items.find(menu_entry_key);
    if (item != m_menu_items.end())
      item->second->activate();
  }

  void set_sensitive(bool sensitive, MenuEntryKey menu_entry_key)
  {
    auto item = m_menu_items.find(menu_entry_key);
    if (item != m_menu_items.end())
      item->second->set_sensitive(sensitive);
  }

 private:
  static char const* top_entry_label(TopEntries top_entry);
};

} // namespace gui
} // namespace gtkmm3
