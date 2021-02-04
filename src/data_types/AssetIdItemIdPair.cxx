#include "sys.h"
#include "AssetIdItemIdPair.h"

constexpr size_t AssetIdItemIdPair::s_number_of_members;

#define xmlrpc_NAMES_ARE_THE_SAME(flags, type, el) #el,

std::array<char const*, AssetIdItemIdPair::s_number_of_members> AssetIdItemIdPair::s_member2name = {
  xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_NAMES_ARE_THE_SAME)
};
