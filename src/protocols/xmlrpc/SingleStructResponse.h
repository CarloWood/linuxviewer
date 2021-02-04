#pragma once

#include "ElementDecoder.h"
#include "MemberDecoder.h"
#include "ArrayDecoder.h"
#include "debug.h"

namespace xmlrpc {

class UnknownMemberDecoder : public ElementDecoder
{
  ElementDecoder* get_struct_decoder() override
  {
    // Implement struct type.
    ASSERT(false);
    return nullptr;
  }

 public:
  template<typename T>
  UnknownMemberDecoder(T& UNUSED_ARG(unknown_member), int UNUSED_ARG(flags) COMMA_CWDEBUG_ONLY(char const* name)) { }
};

struct Unknown
{
  enum members { };
  static constexpr size_t s_numer_of_members = 0;
};

template<>
class MemberDecoder<Unknown> : public UnknownMemberDecoder
{
  using UnknownMemberDecoder::UnknownMemberDecoder;
};

template<>
class ArrayDecoder<Unknown> : public ArrayDecoderBase<Unknown>
{
  using ArrayDecoderBase<Unknown>::ArrayDecoderBase;
};

class SingleStructResponse : public ElementDecoder
{
 private:
  bool m_saw_struct;

 protected:
  SingleStructResponse() : m_saw_struct(false) { }

  ElementDecoder* get_struct_decoder() override;
};

} // namespace xmlrpc
