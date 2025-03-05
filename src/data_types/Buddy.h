#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/initialize.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_Buddy_FOREACH_MEMBER(X) \
  X(int32_t, buddy_rights_has) \
  X(int32_t, buddy_rights_given) \
  X(UUID, buddy_id)

class Buddy
{
 private:
  xmlrpc_Buddy_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members : unsigned char {
    xmlrpc_Buddy_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
