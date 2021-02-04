#pragma once

#include "DecoderBase.h"
#include "IgnoreElement.h"      // Not used here, but needed for all specializatons.
#include "utils/Dictionary.h"
#include "utils/AIAlert.h"
#include <vector>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class ArrayDecoderBase : public DecoderBase<std::vector<T>>
{
 protected:
  T* m_array_element;
  utils::Dictionary<typename T::members, int> m_dictionary;

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

  ElementDecoder* get_struct_decoder() override
  {
    return this;
  }

 public:
  ArrayDecoderBase(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    DecoderBase<std::vector<T>>(member, flags COMMA_CWDEBUG_ONLY(name)), m_array_element(nullptr) { }
};

// This class needs to be specialized for every class T (all of them deriving from ArrayDecoderBase.
template<typename T>
class ArrayDecoder;

} // namespace xmlrpc
