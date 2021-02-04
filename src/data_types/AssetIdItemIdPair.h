#pragma once

#include "UUID.h"
#include "protocols/xmlrpc/ArrayDecoder.h"
#include "protocols/xmlrpc/create_member_decoder.h"
#include "protocols/xmlrpc/macros.h"
#include <array>

#define xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(X) \
  X(0, UUID, asset_id) \
  X(0, UUID, item_id)

class AssetIdItemIdPair
{
 public:
  AssetIdItemIdPair() { }

  enum members {
    xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_ENUMERATOR)
  };

  xmlrpc_AssetIdItemIdPair_FOREACH_MEMBER(XMLRPC_DECLARE_MEMBER)

  xmlrpc::ElementDecoder* get_member_decoder(members member);

 public:
  static constexpr size_t s_number_of_members = XMLRPC_NUMBER_OF_MEMBERS(AssetIdItemIdPair);
  static std::array<char const*, s_number_of_members> s_member2name;
};

namespace xmlrpc {

template<>
class ArrayDecoder<AssetIdItemIdPair> : public ArrayDecoderBase<AssetIdItemIdPair>
{
 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override;

#ifdef CWDEBUG
  ElementDecoder* get_struct_decoder() override
  {
    m_struct_name = "AssetIdItemIdPair";
    return this;
  }
#endif

  using ArrayDecoderBase<AssetIdItemIdPair>::ArrayDecoderBase;
};

} // namespace xmlrpc
