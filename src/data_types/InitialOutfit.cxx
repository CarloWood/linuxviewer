#include "sys.h"
#include "InitialOutfit.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t InitialOutfit::s_number_of_members;

std::array<char const*, InitialOutfit::s_number_of_members> InitialOutfit::s_member2name = {
  xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER_NAME)
};

xmlrpc::ElementDecoder* InitialOutfit::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

namespace xmlrpc {

ElementDecoder* StructDecoder<InitialOutfit>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= InitialOutfit::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  InitialOutfit::members member = static_cast<InitialOutfit::members>(index);
  return m_member.get_member_decoder(member);
}

} // namespace xmlrpc
