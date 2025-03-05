#pragma once

#include "UUID.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_UIConfig_FOREACH_MEMBER(X) \
  X(bool, allow_first_life)

class UIConfig
{
 private:
  xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members : unsigned char {
    xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
