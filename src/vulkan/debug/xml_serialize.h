#pragma once

// XML serialization should only be used in debug mode.
#ifdef CWDEBUG
#include "vk_enums_reflection.h"
#include <vulkan/vulkan.hpp>
#include <type_traits>
#include "vk_utils/print_flags.h"
#include "utils/BitSet.h"
#include "utils/split.h"
#include "debug.h"

#ifdef __LIBXMLCPP_H
// We add specializations that are used by Bridge.
#error "debug/xml_serialize.h must included before xml/Bridge.h"
#endif

// We need read_from_stream because we'll specialize it.
#include "xml/read_from_stream.h"

namespace xml {

template<typename FE>
concept ConceptVkEnumWithToString =
  std::is_enum_v<FE> && vk::details::ConceptHasToString<FE>;

template<ConceptVkEnumWithToString T>
void write_to_stream(std::ostream& os, T& value)
{
  // Even though we checked that T has a to_string, we don't use it because
  // the output must exactly match what works with the read_from_stream below.
  os << vk_enums::enum_name(value);
}

template<ConceptVkEnumWithToString T>
void read_from_stream(std::istream& is, T& value)
{
  std::string value_str;
  std::getline(is, value_str);
  value = vk_enums::enum_cast<T>(value_str).value();
}

// The specialization needs to be declared before it is being used by Bridge.h.
template<typename T>
void write_to_stream(std::ostream& os, vk::Flags<T>& flags)
{
  static_assert(std::is_enum_v<T>, "T must be an enum.");
  using underlying_type = typename vk::Flags<T>::MaskType;
  utils::BitSet<std::make_unsigned_t<underlying_type>> bitmask{
    static_cast<std::make_unsigned_t<underlying_type>>(static_cast<underlying_type>(flags))};
  char const* separator = "";
  for (auto iter = bitmask.begin(); iter != bitmask.end(); ++iter)
  {
    auto flag = *iter;
    os << separator << vk_enums::enum_name(static_cast<T>(flag()));
    separator = "|";
  }
}

template<typename T>
void read_from_stream(std::istream& is, vk::Flags<T>& flags)
{
  static_assert(std::is_enum_v<T>, "T must be an enum.");
  std::string flags_str;
  std::getline(is, flags_str);
  flags = vk::Flags<T>{};
  utils::split(flags_str, '|', [&](std::string_view sv){
      if (!sv.empty())
      {
        std::optional<T> flag{vk_enums::enum_cast<T>(sv)};
        flags |= flag.value();
      }
  });
}

} // namespace xml

#endif // CWDEBUG
