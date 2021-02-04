#include "sys.h"
#include "InitialOutfit.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t InitialOutfit::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, InitialOutfit::s_number_of_members> InitialOutfit::s_member2name = {
  xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};

namespace xmlrpc {

ElementDecoder* StructWrapper<InitialOutfit>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= InitialOutfit::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  InitialOutfit::members member = static_cast<InitialOutfit::members>(index);
  switch (member)
  {
    #define xmlrpc_InitialOutfit_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case InitialOutfit::member_##el: \
      return create_member_wrapper(m_member.m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

} // namespace xmlrpc
