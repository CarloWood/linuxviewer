#pragma once

#include "debug.h"
#include "utils/has_print_on.h"

namespace vulkan {

// Print objects from namespace vulkan, using ADL, if they have a print_on member.
using utils::has_print_on::operator<<;

namespace rendergraph {
using utils::has_print_on::operator<<;
} // namespace rendergraph

namespace shader_builder {
using utils::has_print_on::operator<<;
namespace shader_resource {
using utils::has_print_on::operator<<;
} // namespace descriptor
} // namespace shader_builder

namespace memory {
using utils::has_print_on::operator<<;
} // namespace memory;

namespace descriptor {
using utils::has_print_on::operator<<;
} // namespace descriptor

namespace task {
using utils::has_print_on::operator<<;
} // namespace task

} // namespace vulkan

namespace vk_defaults {
using utils::has_print_on::operator<<;
} // namespace vk_defaults
