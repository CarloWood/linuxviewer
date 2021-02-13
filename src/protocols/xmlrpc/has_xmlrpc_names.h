#pragma once

#include <type_traits>

namespace xmlrpc {

template<typename, typename = void>
constexpr bool has_xmlrpc_names_v = false;

template<typename T>
constexpr bool has_xmlrpc_names_v<T, std::void_t<decltype(T::s_xmlrpc_names)>> = true;

} // namespace xmlrpc
