#pragma once

#include "debug.h"

#define XMLRPC_DECLARE_UNKNOWN(T) \
  struct T { \
    enum members { }; \
    static constexpr size_t s_number_of_members = 0; \
  }; \
  template<> class MemberDecoder<T> : public UnknownMemberDecoder \
  { \
    using UnknownMemberDecoder::UnknownMemberDecoder; \
  }; \
  template<> class ArrayDecoder<T> : public ArrayDecoderBase<T> \
  { \
    using ArrayDecoderBase<T>::ArrayDecoderBase; \
  }

#define XMLRPC_DECLARE_ENUMERATOR(flags, type, el) member_##el,
#define XMLRPC_DECLARE_MEMBER(flags, type, el) type m_##el;
#define XMLRPC_DECLARE_MEMBER_NAME(flags, type, el) #el,
#define XMLRPC_CASE_RETURN(flags, type, el) case member_##el: return #el;
#define XMLRPC_CASE_RETURN_MEMBER_DECODER(flags, type, el) case member_##el: return xmlrpc::create_member_decoder(m_##el, flags COMMA_CWDEBUG_ONLY(#el));
#define XMLRPC_PLUS_ONE(flags, type, el) + 1
#define XMLRPC_NUMBER_OF_MEMBERS(Class) static_cast<size_t>(0 xmlrpc_##Class##_FOREACH_MEMBER(XMLRPC_PLUS_ONE))
