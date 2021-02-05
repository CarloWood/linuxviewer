#pragma once

#include "DecoderBase.h"
#include "IgnoreElement.h"      // Not used here, but needed for all specializatons.
#include "utils/Dictionary.h"
#include "utils/AIAlert.h"
#include <magic_enum.hpp>
#include <vector>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class ArrayOfStructDecoder : public DecoderBase<std::vector<T>>
{
 protected:
  T* m_array_element;
  utils::Dictionary<typename T::members, int> m_dictionary;

 protected:
  ElementDecoder* get_member_decoder(std::string_view const& name) override;

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
  ArrayOfStructDecoder(std::vector<T>& member, int flags COMMA_CWDEBUG_ONLY(char const* name)) :
    DecoderBase<std::vector<T>>(member, flags COMMA_CWDEBUG_ONLY(name)), m_array_element(nullptr) { }
};

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

template<typename T>
ElementDecoder* ArrayOfStructDecoder<T>::get_member_decoder(std::string_view const& name)
{
  int index = m_dictionary.index(name);
  if (index >= magic_enum::enum_count<typename T::members>())
    return &IgnoreElement::s_ignore_element;
  typename T::members member = static_cast<typename T::members>(index);
  return m_array_element->get_member_decoder(member);
}

} // namespace xmlrpc
