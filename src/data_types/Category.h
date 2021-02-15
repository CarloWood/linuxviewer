#pragma once

#include "UUID.h"
#include "evio/protocol/xmlrpc/macros.h"

#define xmlrpc_Category_FOREACH_MEMBER(X) \
  X(std::string, category_name) \
  X(int32_t, category_id)

class Category
{
 private:
  xmlrpc_Category_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_Category_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  evio::protocol::xmlrpc::ElementDecoder* create_member_decoder(members member);

#ifdef CWDEBUG
  void print_on(std::ostream& os) const;
#endif
};
