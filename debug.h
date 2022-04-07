#pragma once

#define NAMESPACE_DEBUG debug
#define NAMESPACE_DEBUG_START namespace debug {
#define NAMESPACE_DEBUG_END }

#include "cwds/debug.h"

#ifdef CWDEBUG
// We use colors for debug output.
#include "threadpool/debug_colors.h"

// The xml submodule doesn't use utils.
#ifdef HAVE_UTILS_CONFIG_H

#include "utils/print_using.h"
#include "utils/QuotedList.h"
namespace libcwd {
// Allow using print_using and QuotedList inside Dout without the utils:: prefix.
using utils::print_using;
using utils::QuotedList;
} // namespace libcwd

// Add support for classes with a print_on method, defined in global namespace.
#include "utils/has_print_on.h"
// Add catch all for global namespace.
using utils::has_print_on::operator<<;

#endif // HAVE_UTILS_CONFIG_H

namespace std {

// For debugging purposes, allow printing vectors.
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

} // namespace std

#if defined(CWDEBUG) && !defined(DOXYGEN)
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
NAMESPACE_DEBUG_CHANNELS_END
#endif

#endif // CWDEBUG
