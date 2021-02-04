#pragma once

#include "protocols/xmlrpc/StructDecoder.h"
#include "protocols/xmlrpc/create_member_decoder.h"
#include "protocols/xmlrpc/macros.h"
#include <array>

#define xmlrpc_LoginFlags_FOREACH_MEMBER(X) \
  X(0, bool, ever_logged_in) \
  X(0, bool, stipend_since_login) \
  X(0, bool, gendered) \
  X(0, bool, daylight_savings)

class LoginFlags
{
 public:
  enum members {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

  xmlrpc::ElementDecoder* get_member_decoder(members member);

  static constexpr size_t s_number_of_members = member_daylight_savings + 1;
  static std::array<char const*, s_number_of_members> s_member2name;
};

namespace xmlrpc {

template<>
class StructDecoder<LoginFlags> : public StructDecoderBase<LoginFlags>
{
 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override;

#ifdef CWDEBUG
  ElementDecoder* get_struct_decoder() override
  {
    m_struct_name = "LoginFlags";
    return this;
  }
#endif

  using StructDecoderBase<LoginFlags>::StructDecoderBase;
};

} // namespace xmlrpc
