#pragma once

#include "MemberDecoder.h"
#include "ArrayDecoder.h"
#include "StructDecoder.h"
#include "debug.h"

namespace xmlrpc {

namespace {

template<typename, typename = void>
constexpr bool has_members_v = false;

template<typename T>
constexpr bool has_members_v<T, std::void_t<typename T::members>> = true;

} // namespace

template<typename T>
ElementDecoder* create_member_decoder(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  if constexpr (has_members_v<T>)      // Is T a <struct>?
    return new StructDecoder<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
  else
    return new MemberDecoder<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<typename T>
ElementDecoder* create_member_decoder(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  if constexpr (has_members_v<T>)      // Is T a <struct>?
    return new ArrayOfStructDecoder<T>{member, flags|2 COMMA_CWDEBUG_ONLY(name)};
  else
    return new ArrayOfMemberDecoder<T>{member, flags|2 COMMA_CWDEBUG_ONLY(name)};
}

} // namespace xmlrpc
