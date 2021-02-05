#pragma once

#include "DecoderBase.h"
#include "IgnoreElement.h"
#include "utils/Dictionary.h"
#include "utils/AIAlert.h"
#include "debug.h"

namespace xmlrpc {

template<typename T>
class StructDecoder : public DecoderBase<T>
{
 protected:
  utils::Dictionary<typename T::members, int> m_dictionary;

 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override;

#ifdef CWDEBUG
  ElementDecoder* get_struct_decoder() override
  {
    if ((this->m_flags & 1))
      THROW_ALERT("Expected <array> before <struct>");
    this->m_struct_name = libcwd::type_info_of<T>().demangled_name();
    return StructDecoder<T>::get_struct_decoder();
  }
#endif

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
    for (size_t i = 0; i < T::s_number_of_members; ++i)
      m_dictionary.add(static_cast<typename T::members>(i), T::s_member2name[i]);
  }
};

template<typename T>
ElementDecoder* StructDecoder<T>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= T::s_number_of_members)
    return &IgnoreElement::s_ignore_element;
  typename T::members member = static_cast<typename T::members>(index);
  return this->m_member.get_member_decoder(member);
}

} // namespace xmlrpc
