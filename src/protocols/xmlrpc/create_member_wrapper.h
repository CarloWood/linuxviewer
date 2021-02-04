#pragma once

#include "MemberWrapper.h"
#include "ArrayWrapper.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
ElementDecoder* create_member_wrapper(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new MemberWrapper<T>{member, flags COMMA_CWDEBUG_ONLY(name)};
}

template<typename T>
ElementDecoder* create_member_wrapper(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name))
{
  //FIXME: this is not freed anywhere yet.
  return new ArrayWrapper<T>{member, flags|2 COMMA_CWDEBUG_ONLY(name)};
}

} // namespace xmlrpc
