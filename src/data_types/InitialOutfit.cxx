#include "sys.h"
#include "InitialOutfit.h"

constexpr size_t InitialOutfit::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, InitialOutfit::s_number_of_members> InitialOutfit::s_member2name = {
  xmlrpc_InitialOutfit_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};
