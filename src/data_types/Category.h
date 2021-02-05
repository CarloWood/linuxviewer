#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_Category_FOREACH_MEMBER(X) \
  X(0, std::string, category_name) \
  X(0, int32_t, category_id)

class Category
{
 private:
  xmlrpc_Category_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_Category_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
