#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(X) \
  X(0, UUID, asset_id) \
  X(0, UUID, item_id)

class AssetIdItemIdPair
{
 private:
  xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
