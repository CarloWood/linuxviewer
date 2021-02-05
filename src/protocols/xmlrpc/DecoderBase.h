#pragma once

#include "ElementDecoder.h"
#include "utils/AIAlert.h"
#include <string>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class DecoderBase : public ElementDecoder
{
 protected:
  T& m_member;
  int m_flags;
#ifdef CWDEBUG
  std::string m_name;
#endif

  ElementDecoder* get_struct_decoder() override
  {
    if ((this->m_flags & 1))
      THROW_ALERT("Expected <array> before <struct>");
#ifdef CWDEBUG
    this->m_struct_name = libcwd::type_info_of<T>().demangled_name();
#endif
    return this;
  }

 public:
  DecoderBase(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) : m_member(member), m_flags(flags), m_name(name) { }
};

} // namespace xmlrpc
