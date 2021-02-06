#pragma once

#include "DecoderBase.h"
#include "utils/AIAlert.h"
#include <vector>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class ArrayOfMemberDecoder : public DecoderBase<std::vector<T>>
{
 protected:
  T* m_array_element;

 private:
  ElementDecoder* get_array_decoder() override
  {
    if (!(this->m_flags & 2))
      THROW_ALERT("Unexpected <array>");
    // Reset the flags because we should only skip one <array>.
    this->m_flags &= ~2;
    return this;
  }

  void got_data() override
  {
    this->m_member.emplace_back();
    m_array_element = &this->m_member.back();
  }

  void got_characters(std::string_view const& data) override
  {
    initialize(*m_array_element, data);
    Dout(dc::notice, "Initialized new array element with '" << data << "'.");
  }

 public:
  ArrayOfMemberDecoder(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    DecoderBase<std::vector<T>>(member, flags COMMA_CWDEBUG_ONLY(name)), m_array_element(nullptr) { }
};

} // namespace xmlrpc
