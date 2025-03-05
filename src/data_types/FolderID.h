#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/initialize.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_FolderID_FOREACH_MEMBER(X) \
  X(UUID, folder_id)

class FolderID
{
 private:
  xmlrpc_FolderID_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members : unsigned char {
    xmlrpc_FolderID_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
