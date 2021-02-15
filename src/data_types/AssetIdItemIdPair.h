#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/initialize.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(X) \
  X(UUID, asset_id) \
  X(UUID, item_id)

class AssetIdItemIdPair
{
 private:
  xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
