#pragma once

#include "DecoderBase.h"
#include "utils/Dictionary.h"
#include "utils/AIAlert.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
class StructDecoderBase : public DecoderBase<T>
{
 protected:
  utils::Dictionary<typename T::members, int> m_dictionary;

 protected:
  ElementDecoder* get_struct_decoder() override
  {
    if ((this->m_flags & 1))
      THROW_ALERT("Expected <array> before <struct>");
    return this;
  }

 private:
  ElementDecoder* get_array_decoder() override
  {
    // The 1 bit must be set in order to skip an <array>.
    if (!(this->m_flags & 1))
      THROW_ALERT("Unexpected <array>");
    return this;
  }

  void got_data() override
  {
    // Reset the flags because we should only skip one <array><data>
    this->m_flags &= ~1;
  }

#ifdef CWDEBUG
  void got_member_type(xmlrpc::data_type type, char const* struct_name) override
  {
  }
#endif

 public:
  StructDecoderBase(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    DecoderBase<T>(member, flags COMMA_CWDEBUG_ONLY(name))
  {
    for (size_t i = 0; i < T::s_number_of_members; ++i)
      m_dictionary.add(static_cast<typename T::members>(i), T::s_member2name[i]);
  }
};

// This class needs to be specialized for every class T (all of them deriving from StructDecoderBase.
template<typename T>
class StructDecoder;

} // namespace xmlrpc
