#pragma once

#include "debug.h"
#include "utils/has_print_on.h"

namespace vulkan {

// Print objects from namespace vulkan, using ADL, if they have a print_on member.
using utils::has_print_on::operator<<;

namespace rendergraph {
using utils::has_print_on::operator<<;
} // namespace rendergraph

} // namespace vulkan

namespace vk_defaults {
using utils::has_print_on::operator<<;
} // namespace vk_defaults
