#pragma once

#define NAMESPACE_DEBUG debug
#define NAMESPACE_DEBUG_START namespace debug {
#define NAMESPACE_DEBUG_END }
#include "cwds/debug.h"

#include "utils/print_using.h"
namespace libcwd {
// Allow using PrintUsing inside Dout without the utils:: prefix.
using utils::print_using;
} // namespace libcwd
