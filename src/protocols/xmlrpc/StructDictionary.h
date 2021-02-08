#pragma once

#include "IgnoreElement.h"
#include "utils/Dictionary.h"
#include "threadsafe/AIReadWriteMutex.h"
#include "threadsafe/aithreadsafe.h"
#include <magic_enum.hpp>
#include "debug.h"

namespace xmlrpc {

template<typename T>
class StructDictionary {
 protected:
  using dictionary_type = aithreadsafe::Wrapper<utils::Dictionary<typename T::members, int>, aithreadsafe::policy::ReadWrite<AIReadWriteMutex>>;
  using UnknownMember = typename dictionary_type::data_type::NonExistingWord;

  static dictionary_type s_dictionary;

  typename T::members get_member(std::string_view const& name);

  StructDictionary();
};

template<typename T>
typename StructDictionary<T>::dictionary_type StructDictionary<T>::s_dictionary;

template<typename T>
StructDictionary<T>::StructDictionary()
{
  // Only initialize the (static) dictionary once!
  // See https://stackoverflow.com/a/36596693/1487069
  static bool once = [](){
    typename dictionary_type::wat dictionary_w(s_dictionary);
    for (int i = 0; i < magic_enum::enum_count<typename T::members>(); ++i)
    {
      typename T::members member = static_cast<typename T::members>(i);
      std::string_view member_name = magic_enum::enum_name(member);
      // All member names start with "member_".
      ASSERT(member_name.substr(0, 7) == "member_");
      member_name.remove_prefix(7);
      dictionary_w->add(member, std::move(member_name));
    }
    return true;
  } ();
}

// Throws StructDictionary<T>::UnknownMember if 'name' is not in T::members.
template<typename T>
typename T::members StructDictionary<T>::get_member(std::string_view const& name)
{
  int index;
  typename dictionary_type::rat dictionary_r(s_dictionary);
  if (name.find('-') == std::string_view::npos)
    index = dictionary_r->index(name);
  else
  {
    std::string converted_name{name};
    std::replace(converted_name.begin(), converted_name.end(), '-', '_'); // Replace al '-' with '_'.
    index = dictionary_r->index(converted_name);
  }
  return static_cast<typename T::members>(index);
}

} // namespace xmlrpc
