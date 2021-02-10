#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_UIConfig_FOREACH_MEMBER(X) \
  X(bool, allow_first_life)

class UIConfig
{
 private:
  xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_UIConfig_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* create_member_decoder(members member);
};
