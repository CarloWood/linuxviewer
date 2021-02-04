#include "sys.h"
#include "AssetIdItemIdPair.h"

constexpr size_t AssetIdItemIdPair::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, AssetIdItemIdPair::s_number_of_members> AssetIdItemIdPair::s_member2name = {
  xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};

namespace xmlrpc {

ElementDecoder* ArrayWrapper<AssetIdItemIdPair>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= AssetIdItemIdPair::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  AssetIdItemIdPair::members member = static_cast<AssetIdItemIdPair::members>(index);
  switch (member)
  {
    #define xmlrpc_AssetIdItemIdPair_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case AssetIdItemIdPair::member_##el: \
      return create_member_wrapper(m_array_element->m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_AssetIdItemIdPair_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

} // namespace xmlrpc
