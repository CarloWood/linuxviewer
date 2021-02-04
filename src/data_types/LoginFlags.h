#pragma once

#include <array>

#define xmlrpc_LoginFlags_FOREACH_ELEMENT(X) \
  X(0, bool, ever_logged_in) \
  X(0, bool, stipend_since_login) \
  X(0, bool, gendered) \
  X(0, bool, daylight_savings)

#define xmlrpc_LoginFlags_ENUMERATOR_NAME_COMMA(flags, type, el) member_##el,
#define xmlrpc_LoginFlags_DECLARATION(flags, type, el) type m_##el;

class LoginFlags
{
 public:
  LoginFlags() { }

  enum members {
    xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_ENUMERATOR_NAME_COMMA)
  };

 public:
  xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_DECLARATION)

 public:
#if 0
  switch (member)
  {
    #define xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER(flags, type, el) \
    case LoginFlags::member_##el: \
      return create_xmlrpc_member_wrapper(m_member.m_##el, flags COMMA_CWDEBUG_ONLY(#el));

    xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_CREATE_XMLRPC_MEMBER_WRAPPER)
  }
#endif

 public:
  static constexpr size_t s_number_of_members = member_daylight_savings + 1;
  static std::array<char const*, s_number_of_members> s_member2name;
};
