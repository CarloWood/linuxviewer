#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_InventoryFolder_FOREACH_MEMBER(X) \
  X(0, std::string, name) \
  X(0, int32_t, version) \
  X(0, UUID, folder_id) \
  X(0, int32_t, type_default) \
  X(0, UUID, parent_id)

class InventoryFolder
{
 private:
  xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_InventoryFolder_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
