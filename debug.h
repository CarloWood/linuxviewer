#pragma once

#include "cwds/debug.h"

#ifdef CWDEBUG
#ifdef DEBUG_THREADPOOL_COLORS
// We use colors for debug output.
#include "threadpool/debug_colors.h"
#endif

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

#ifndef DOXYGEN
NAMESPACE_DEBUG_CHANNELS_START
extern channel_ct vulkan;
extern channel_ct shaderresource;
NAMESPACE_DEBUG_CHANNELS_END
#endif

#endif // CWDEBUG
