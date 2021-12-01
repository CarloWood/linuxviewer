#pragma once

#include <cstddef>

namespace vk_utils {

// See https://stackoverflow.com/a/37931904/1487069
template<std::size_t N, typename T, typename... types>
struct get_Nth_type
{
  using type = typename get_Nth_type<N - 1, types...>::type;
};

template<typename T, typename... types>
struct get_Nth_type<0, T, types...>
{
  using type = T;
};

// See https://awesomekling.github.io/Serenity-C++-patterns-The-Badge
template<typename... types>
class Badge
{
  static constexpr std::size_t S = sizeof...(types);
  friend typename get_Nth_type<0 % S, types...>::type;
  friend typename get_Nth_type<1 % S, types...>::type;
  friend typename get_Nth_type<2 % S, types...>::type;
  friend typename get_Nth_type<3 % S, types...>::type;

  Badge() {}
};

} // namespace vk_utils
