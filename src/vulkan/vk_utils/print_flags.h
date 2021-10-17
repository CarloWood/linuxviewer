#pragma once

#include <vulkan/vulkan.hpp>
#include <type_traits>
#include <iostream>

namespace vk {

namespace details {

template<typename>
struct is_vk_flags : public std::false_type {};

template<typename BitType>
struct is_vk_flags<vk::Flags<BitType>> : public std::true_type
{
  using flags_type = vk::Flags<BitType>;
};

template<typename Flags>
concept ConceptIsVkFlags = is_vk_flags<Flags>::value && std::is_same_v<Flags, typename is_vk_flags<Flags>::flags_type>;

template<typename FE>
concept ConceptHasToString = requires(FE flags_or_enum)
{
  ::vk::to_string(flags_or_enum);
};

template<typename FE>
concept ConceptVkFlagsWithToString =
  (ConceptIsVkFlags<FE> || std::is_enum_v<FE>) && ConceptHasToString<FE>;

} // namespace details

// Print enum (class) types and Flags using vk::to_string.
template<typename Flags>
std::ostream& operator<<(std::ostream& os, Flags flags) requires details::ConceptVkFlagsWithToString<Flags>
{
  os << to_string(flags);
  return os;
}

} // namespace vk

