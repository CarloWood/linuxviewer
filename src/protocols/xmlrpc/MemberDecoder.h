#pragma once

#include "DecoderBase.h"
#include "initialize.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
class MemberDecoder : public DecoderBase<T>
{
  void got_characters(std::string_view const& data) override
  {
    initialize(this->m_member, data);
  }

 public:
  using DecoderBase<T>::DecoderBase;
};

} // namespace xmlrpc
