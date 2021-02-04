#pragma once

#include "UUID.h"
#include <array>

#define xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(X) \
  X(0, UUID, asset_id) \
  X(0, UUID, item_id)

#define xmlrpc_AssetIdItemIdPair_ENUMERATOR_NAME_COMMA(flags, type, el) member_##el,
#define xmlrpc_AssetIdItemIdPair_DECLARATION(flags, type, el) type m_##el;
#define xmlrpc_AssetIdItemIdPair_PLUS_ONE(flags, type, el) + 1
#define xmlrpc_AssetIdItemIdPair_NUMBER_OF_ELEMENTS(Class) static_cast<size_t>(0 xmlrpc_##Class##_FOREACH_ELEMENT(xmlrpc_AssetIdItemIdPair_PLUS_ONE))

class AssetIdItemIdPair
{
 public:
  AssetIdItemIdPair() { }

  enum members {
    xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_AssetIdItemIdPair_ENUMERATOR_NAME_COMMA)
  };

  xmlrpc_AssetIdItemIdPair_FOREACH_ELEMENT(xmlrpc_AssetIdItemIdPair_DECLARATION)

 public:
  static constexpr size_t s_number_of_members = xmlrpc_AssetIdItemIdPair_NUMBER_OF_ELEMENTS(AssetIdItemIdPair);
  static std::array<char const*, s_number_of_members> s_member2name;
};
