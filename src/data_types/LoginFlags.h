#pragma once

#include "protocols/xmlrpc/macros.h"

#define xmlrpc_LoginFlags_FOREACH_MEMBER(X) \
  X(bool, ever_logged_in) \
  X(bool, stipend_since_login) \
  X(bool, gendered) \
  X(bool, daylight_savings)

class LoginFlags
{
 private:
  xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* create_member_decoder(members member);
};
