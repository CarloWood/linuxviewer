#pragma once

#include "Gender.h"
#include <string>
#include <array>

#define xmlrpc_InitialOutfit_FOREACH_ELEMENT(X) \
  X(0, std::string, folder_name) \
  X(0, Gender, gender)

#define xmlrpc_InitialOutfit_ENUMERATOR_NAME_COMMA(flags, type, el) member_##el,
#define xmlrpc_InitialOutfit_DECLARATION(flags, type, el) type m_##el;
#define xmlrpc_InitialOutfit_PLUS_ONE(flags, type, el) + 1
#define xmlrpc_InitialOutfit_NUMBER_OF_ELEMENTS(Class) static_cast<size_t>(0 xmlrpc_##Class##_FOREACH_ELEMENT(xmlrpc_InitialOutfit_PLUS_ONE))

class InitialOutfit
{
 public:
  InitialOutfit() { }

  enum members {
    xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_ENUMERATOR_NAME_COMMA)
  };

  xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_InitialOutfit_DECLARATION)

 public:
  static constexpr size_t s_number_of_members = xmlrpc_InitialOutfit_NUMBER_OF_ELEMENTS(InitialOutfit);
  static std::array<char const*, s_number_of_members> s_member2name;
};
