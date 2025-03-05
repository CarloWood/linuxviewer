#pragma once

#include "evio/protocol/xmlrpc/macros.h"
#ifdef CWDEBUG
#include <iosfwd>
#endif

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
  enum members : unsigned char {
    xmlrpc_LoginFlags_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
