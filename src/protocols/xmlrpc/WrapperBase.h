#pragma once

#include "ElementDecoder.h"
#include <string>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class WrapperBase : public ElementDecoder
{
 protected:
  T& m_member;
  int m_flags;
#ifdef CWDEBUG
  std::string m_name;
#endif

 public:
  WrapperBase(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) : m_member(member), m_flags(flags), m_name(name) { }
};

} // namespace xmlrpc
