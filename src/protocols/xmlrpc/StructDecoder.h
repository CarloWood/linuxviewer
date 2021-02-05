#pragma once

#include "DecoderBase.h"
#include "IgnoreElement.h"
#include "utils/Dictionary.h"
#include "utils/AIAlert.h"
#include <magic_enum.hpp>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class StructDecoder : public DecoderBase<T>
{
 protected:
  utils::Dictionary<typename T::members, int> m_dictionary;

 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override;

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
  StructDecoder(T& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    DecoderBase<T>(member, flags COMMA_CWDEBUG_ONLY(name))
  {
    for (int i = 0; i < magic_enum::enum_count<typename T::members>(); ++i)
    {
      typename T::members member = static_cast<typename T::members>(i);
      m_dictionary.add(member, magic_enum::enum_name(member));
    }
  }
};

template<typename T>
ElementDecoder* StructDecoder<T>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= magic_enum::enum_count<typename T::members>())
    return &IgnoreElement::s_ignore_element;
  typename T::members member = static_cast<typename T::members>(index);
  return this->m_member.get_member_decoder(member);
}

} // namespace xmlrpc
