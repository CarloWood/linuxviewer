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

#include "utils/has_print_on.h"

template<typename T>
std::enable_if_t<utils::has_print_on<T const>, std::ostream&>
operator<<(std::ostream& os, T const& data)
{
  data.print_on(os);
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T> const& v)
{
  os << '{';
  char const* prefix = "";
  for (auto&& element : v)
  {
    os << prefix << element;
    prefix = ", ";
  }
  return os << '}';
}

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif

#endif // CWDEBUG
