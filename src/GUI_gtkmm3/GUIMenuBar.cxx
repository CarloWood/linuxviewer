#include "sys.h"
#include "GUIMenuBar.h"
#include "GUIWindow.h"
#include "GUIApplication.h"
#include "utils/macros.h"

using namespace menu_keys;

namespace gtkmm3 {

//static
char const* GUIMenuBar::top_entry_label(TopEntries top_entry)
{
  switch (top_entry)
  {
    AI_CASE_RETURN(File);
    // Suppress compiler warning.
    case number_of_top_entries:
      break;
  }
  return "";
}

GUIMenuBar::GUIMenuBar(GUIWindow* main_window)
{
  // Prepare top level entries.
  for (int tl = 0; tl < number_of_top_entries; ++tl)
  {
    TopEntries te = static_cast<TopEntries>(tl);
    auto tl_item = Gtk::manage(new Gtk::ImageMenuItem(top_entry_label(te), true));
    append(*tl_item);
    m_submenus[te] = Gtk::manage(new Gtk::Menu);
    tl_item->set_submenu(*m_submenus[te]);
  }

  main_window->append_menu_entries(this);
  main_window->application().append_menu_entries(this);
}

} // namespace gtkmm3
