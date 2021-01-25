#pragma once

#define NAMESPACE_DEBUG debug
#define NAMESPACE_DEBUG_START namespace debug {
#define NAMESPACE_DEBUG_END }
#include "cwds/debug.h"

#ifdef CWDEBUG
#include "utils/print_using.h"
namespace libcwd {
// Allow using PrintUsing inside Dout without the utils:: prefix.
using utils::print_using;
} // namespace libcwd

namespace {

template<typename T, typename = void>
constexpr bool has_print_on = false;

template<typename T>
constexpr bool has_print_on<T, std::void_t<decltype(std::declval<T>().print_on(std::declval<std::ostream&>()))>> = true;

} // namespace

template<typename T>
std::enable_if_t<has_print_on<T const>, std::ostream&>
operator<<(std::ostream& os, T const& data)
{
  data.print_on(os);
  return os;
}
#endif
