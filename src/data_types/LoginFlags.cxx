#include "sys.h"
#include "LoginFlags.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t LoginFlags::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, LoginFlags::s_number_of_members> LoginFlags::s_member2name = {
  xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};

namespace xmlrpc {

ElementDecoder* StructWrapper<LoginFlags>::get_member(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= LoginFlags::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  LoginFlags::members member = static_cast<LoginFlags::members>(index);
  switch (member)
  {
    #define xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case LoginFlags::member_##el: \
      return create_member_wrapper(m_member.m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
}

} // namespace xmlrpc
