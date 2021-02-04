#pragma once

#include "protocols/xmlrpc/StructWrapper.h"
#include "protocols/xmlrpc/create_member_wrapper.h"
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
  enum members {
    xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_ENUMERATOR_NAME_COMMA)
  };

  xmlrpc_LoginFlags_FOREACH_ELEMENT(xmlrpc_LoginFlags_DECLARATION)

  static constexpr size_t s_number_of_members = member_daylight_savings + 1;
  static std::array<char const*, s_number_of_members> s_member2name;
};

namespace xmlrpc {

template<>
class StructWrapper<LoginFlags> : public StructWrapperBase<LoginFlags>
{
 protected:
  ElementDecoder* get_member(std::string_view const& name) override;

#ifdef CWDEBUG
  ElementDecoder* get_struct() override
  {
    m_struct_name = "LoginFlags";
    return this;
  }
#endif

  using StructWrapperBase<LoginFlags>::StructWrapperBase;
};

template<>
inline ElementDecoder* create_member_wrapper(LoginFlags& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new StructWrapper<LoginFlags>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

} // namespace xmlrpc
