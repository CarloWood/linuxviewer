#include "sys.h"
#include "LoginFlags.h"
#include "protocols/xmlrpc/IgnoreElement.h"

constexpr size_t LoginFlags::s_number_of_members;

std::array<char const*, LoginFlags::s_number_of_members> LoginFlags::s_member2name = {
  xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER_NAME)
};

xmlrpc::ElementDecoder* LoginFlags::get_member_decoder(members member)
{
  switch (member)
  {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_CASE_RETURN_MEMBER_DECODER)
  }
}

namespace xmlrpc {

ElementDecoder* StructDecoder<LoginFlags>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= LoginFlags::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  LoginFlags::members member = static_cast<LoginFlags::members>(index);
  return m_member.get_member_decoder(member);
}

} // namespace xmlrpc
