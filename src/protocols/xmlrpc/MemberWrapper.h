#pragma once

#include "WrapperBase.h"
#include "initialize.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
class MemberWrapper : public WrapperBase<T>
{
  void got_characters(std::string_view const& data) override
  {
    initialize(this->m_member, data);
    Dout(dc::notice, "Initialized member 'm_" << this->m_name << "' with '" << data << "'.");
  }

 public:
  using WrapperBase<T>::WrapperBase;
};

} // namespace xmlrpc
