#pragma once

#include "protocols/xmlrpc/macros.h"

#define xmlrpc_LoginFlags_FOREACH_MEMBER(X) \
  X(0, bool, ever_logged_in) \
  X(0, bool, stipend_since_login) \
  X(0, bool, gendered) \
  X(0, bool, daylight_savings)

class LoginFlags
{
 private:
  xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
