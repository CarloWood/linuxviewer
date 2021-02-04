#pragma once

#include "ElementDecoder.h"
#include "MemberWrapper.h"
#include "ArrayWrapper.h"
#include "debug.h"

namespace xmlrpc {

class UnknownMemberWrapper : public ElementDecoder
{
  ElementDecoder* get_struct() override
  {
    // Implement struct type.
    ASSERT(false);
    return nullptr;
  }

 public:
  template<typename T>
  UnknownMemberWrapper(T& UNUSED_ARG(unknown_member), int UNUSED_ARG(flags) COMMA_CWDEBUG_ONLY(char const* name)) { }
};

struct Unknown
{
  enum members { };
  static constexpr size_t s_numer_of_members = 0;
};

template<>
class MemberWrapper<Unknown> : public UnknownMemberWrapper
{
  using UnknownMemberWrapper::UnknownMemberWrapper;
};

template<>
class ArrayWrapper<Unknown> : public ArrayWrapperBase<Unknown>
{
  using ArrayWrapperBase<Unknown>::ArrayWrapperBase;
};

class SingleStructResponse : public ElementDecoder
{
 private:
  bool m_saw_struct;

 protected:
  SingleStructResponse() : m_saw_struct(false) { }

  ElementDecoder* get_struct() override;
};

} // namespace xmlrpc
