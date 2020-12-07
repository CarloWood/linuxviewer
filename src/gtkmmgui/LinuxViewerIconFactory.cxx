#include "sys.h"
#include "LinuxViewerIconFactory.h"
#include "debug.h"

//static
std::array<LinuxViewerIconFactory::icon_info_st, menu_keys::number_of_custom_icons> LinuxViewerIconFactory::s_icon_info = {{
//  { "pixmaps/export.png", "export", "Export" },
}};

namespace menu_keys {

std::string get_label(MenuEntryWithoutIconId menu_entry_id)
{
  switch (menu_entry_id)
  {
  }
  return "Unknown label";
}

} // namespace menukeys

LinuxViewerIconFactory::LinuxViewerIconFactory()
{
  Glib::RefPtr<Gtk::IconFactory> factory = Gtk::IconFactory::create();
  for (int i = 0; i < s_icon_info.size(); ++i)
  {
    Gtk::IconSource source;
    try
    {
      Glib::RefPtr<Gdk::Pixbuf> const& pixbuf = Gdk::Pixbuf::create_from_file(s_icon_info[i].filepath);
      source.set_pixbuf(pixbuf);
    }
    catch(Glib::Exception const& ex)
    {
      DoutFatal(dc::fatal, ex.what());
    }
    source.set_size(Gtk::ICON_SIZE_SMALL_TOOLBAR);
    source.set_size_wildcarded(); // Icon may be scaled.
    Glib::RefPtr<Gtk::IconSet> icon_set = Gtk::IconSet::create();
    icon_set->add_source(source);
    Gtk::StockID const icon_id(s_icon_info[i].id);
    factory->add(icon_id, icon_set);
    Gtk::Stock::add(Gtk::StockItem(icon_id, s_icon_info[i].label));
  }
  factory->add_default();
  for (int i = 0; i < s_icon_info.size(); ++i)
    m_icon_ids[i] = Gtk::StockID(s_icon_info[i].id);
}
