#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/initialize.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_InventoryFolder_FOREACH_MEMBER(X) \
  X(std::string, name) \
  X(int32_t, version) \
  X(UUID, folder_id) \
  X(int32_t, type_default) \
  X(UUID, parent_id)

class InventoryFolder
{
 private:
  xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members : unsigned char {
    xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
