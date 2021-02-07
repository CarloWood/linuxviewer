#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_Buddy_FOREACH_MEMBER(X) \
  X(0, int32_t, buddy_rights_has) \
  X(0, int32_t, buddy_rights_given) \
  X(0, UUID, buddy_id)

class Buddy
{
 private:
  xmlrpc_Buddy_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_Buddy_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* create_member_decoder(members member);
};
