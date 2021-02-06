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
  //FIXME: this is not freed anywhere yet.
  if constexpr (has_members_v<T>)      // Is T a <struct>?
  {
    Dout(dc::notice, ">>>" << sizeof(StructDecoder<T>));
    return new StructDecoder<T>{member, flags};
  }
  else
  {
    Dout(dc::notice, ">>>" << sizeof(MemberDecoder<T>));
    return new MemberDecoder<T>{member, flags};
  }
}

template<typename T>
ElementDecoder* create_member_decoder(std::vector<T>& member, int flags)
{
  //FIXME: this is not freed anywhere yet.
  if constexpr (has_members_v<T>)      // Is T a <struct>?
  {
    Dout(dc::notice, ">>>" << sizeof(ArrayOfStructDecoder<T>));
    return new ArrayOfStructDecoder<T>{member, flags|2};
  }
  else
  {
    Dout(dc::notice, ">>>" << sizeof(ArrayOfMemberDecoder<T>));
    return new ArrayOfMemberDecoder<T>{member, flags|2};
  }
}

} // namespace xmlrpc
