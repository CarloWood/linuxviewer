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

  ElementDecoder* get_struct_decoder() override
  {
    if ((this->m_flags & 1))
      THROW_ALERT("Expected <array> before <struct>");
    return this;
  }

 public:
  DecoderBase(T& member, int flags) : m_member(member), m_flags(flags) { }
};

} // namespace xmlrpc
