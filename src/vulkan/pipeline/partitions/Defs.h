#pragma once

#include "utils/BitSet.h"

// Externally used types and constants.

namespace vulkan::pipeline::partitions {

using elements_t = utils::BitSet<uint64_t>;
constexpr int8_t max_number_of_elements = 8 * sizeof(elements_t::mask_type);
using partition_count_t = unsigned long;

} // namespace vulkan::pipeline::partitions
