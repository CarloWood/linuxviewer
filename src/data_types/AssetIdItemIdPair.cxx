#include "sys.h"
#include "AssetIdItemIdPair.h"

constexpr size_t AssetIdItemIdPair::s_number_of_members;

std::array<char const*, AssetIdItemIdPair::s_number_of_members> AssetIdItemIdPair::s_member2name = {
  xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER_NAME)
};

xmlrpc::ElementDecoder* AssetIdItemIdPair::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

namespace xmlrpc {

ElementDecoder* ArrayDecoder<AssetIdItemIdPair>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= AssetIdItemIdPair::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  AssetIdItemIdPair::members member = static_cast<AssetIdItemIdPair::members>(index);
  return m_array_element->get_member_decoder(member);
}

} // namespace xmlrpc
