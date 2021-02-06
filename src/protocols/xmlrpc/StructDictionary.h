#pragma once

#include "IgnoreElement.h"
#include "utils/Dictionary.h"
#include <magic_enum.hpp>
#include "debug.h"

namespace xmlrpc {

struct UnknownMember : std::exception
{
};

template<typename T>
class StructDictionary {
 protected:
  utils::Dictionary<typename T::members, int> m_dictionary;

  typename T::members get_member(std::string_view const& name);

  StructDictionary();
};

template<typename T>
StructDictionary<T>::StructDictionary()
{
  for (int i = 0; i < magic_enum::enum_count<typename T::members>(); ++i)
  {
    typename T::members member = static_cast<typename T::members>(i);
    std::string_view member_name = magic_enum::enum_name(member);
    // All member names start with "member_".
    ASSERT(member_name.substr(0, 7) == "member_");
    member_name.remove_prefix(7);
    m_dictionary.add(member, std::move(member_name));
  }
}

template<typename T>
typename T::members StructDictionary<T>::get_member(std::string_view const& name)
{
  int index;
  if (name.find('-') == std::string_view::npos)
    index = m_dictionary.index(name);
  else
  {
    std::string converted_name{name};
    std::replace(converted_name.begin(), converted_name.end(), '-', '_'); // Replace al '-' with '_'.
    index = m_dictionary.index(converted_name);
  }
  if (index >= magic_enum::enum_count<typename T::members>())
    throw UnknownMember{};
  return static_cast<typename T::members>(index);
}

} // namespace xmlrpc
