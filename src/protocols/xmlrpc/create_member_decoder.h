#pragma once

#include "MemberDecoder.h"
#include "ArrayDecoder.h"
#include "StructDecoder.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
ElementDecoder* create_member_decoder(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  if ((flags & 1))      // Is it an <struct>?
    return new StructDecoder<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
  return new MemberDecoder<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<typename T>
ElementDecoder* create_member_decoder(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new ArrayDecoder<T>{member, flags|2 COMMA_CWDEBUG_ONLY(name)};
}

} // namespace xmlrpc
