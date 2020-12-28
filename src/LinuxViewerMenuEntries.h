#pragma once

#include <string>

namespace menu_keys {

// Main menu entries.
enum TopEntries
{
  File,
  number_of_top_entries
};

enum MenuEntryWithIconId
{
  number_of_custom_icons
};

enum MenuEntryWithoutIconId
{
};

std::string get_label(MenuEntryWithoutIconId menu_entry_id);

} // namespace menu_keys
