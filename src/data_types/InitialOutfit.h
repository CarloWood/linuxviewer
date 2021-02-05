#pragma once

#include <string>
#include "Gender.h"
#include "protocols/xmlrpc/macros.h"

#define xmlrpc_InitialOutfit_FOREACH_MEMBER(X) \
  X(0, std::string, folder_name) \
  X(0, Gender, gender)

class InitialOutfit
{
 private:
  xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

 public:
  enum members {
    xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc::ElementDecoder* get_member_decoder(members member);
};
