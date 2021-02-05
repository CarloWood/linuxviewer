#pragma once

#include "Gender.h"
#include "protocols/xmlrpc/StructDecoder.h"
#include "protocols/xmlrpc/create_member_decoder.h"
#include "protocols/xmlrpc/macros.h"
#include <string>
#include <array>

#define xmlrpc_InitialOutfit_FOREACH_MEMBER(X) \
  X(0, std::string, folder_name) \
  X(0, Gender, gender)

class InitialOutfit
{
 public:
  InitialOutfit() { }

  enum members {
    xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc_InitialOutfit_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

  xmlrpc::ElementDecoder* get_member_decoder(members member);

 public:
  static constexpr size_t s_number_of_members = XMLRPC_NUMBER_OF_MEMBERS(InitialOutfit);
  static std::array<char const*, s_number_of_members> s_member2name;
};
