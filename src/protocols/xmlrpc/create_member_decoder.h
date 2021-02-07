#pragma once

#include "StructDecoder.h"
#include "MemberDecoder.h"
#include "ArrayOfStructDecoder.h"
#include "ArrayOfMemberDecoder.h"
#include "debug.h"

namespace xmlrpc {

namespace {

template<typename, typename = void>
constexpr bool has_members_v = false;

template<typename T>
constexpr bool has_members_v<T, std::void_t<typename T::members>> = true;

} // namespace

template<typename T>
ElementDecoder* create_member_decoder(T& member, int flags)
{
  if constexpr (has_members_v<T>)      // Is T a <struct>?
    return new (ElementDecoder::s_pool) StructDecoder<T>{member, flags};
  else
    return new (ElementDecoder::s_pool) MemberDecoder<T>{member, flags};
}

template<typename T>
ElementDecoder* create_member_decoder(std::vector<T>& member, int flags)
{
  if constexpr (has_members_v<T>)      // Is T a <struct>?
    return new (ElementDecoder::s_pool) ArrayOfStructDecoder<T>{member, flags|2};
  else
    return new (ElementDecoder::s_pool) ArrayOfMemberDecoder<T>{member, flags|2};
}

} // namespace xmlrpc
