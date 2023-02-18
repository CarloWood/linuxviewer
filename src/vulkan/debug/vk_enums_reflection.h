#pragma once

#include <string_view>
#include <type_traits>
#include <optional>

namespace vk_enums {
namespace detail {

template<typename T, typename R = void>
using enable_if_enum_t = std::enable_if_t<std::is_enum_v<std::decay_t<T>>, R>;

} // namespace detail

template<typename E>
[[nodiscard]] auto enum_name(E value) noexcept -> detail::enable_if_enum_t<E, std::string_view>;

template<typename E>
[[nodiscard]] auto enum_cast(std::string_view value) noexcept -> detail::enable_if_enum_t<E, std::optional<std::decay_t<E>>>;

} // namespace vk_enums
