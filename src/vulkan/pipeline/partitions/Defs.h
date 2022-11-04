#pragma once

#include "utils/BitSet.h"
#include <array>

// Externally used types and constants.

namespace vulkan::pipeline::partitions {

using elements_t = utils::BitSet<uint64_t>;
constexpr int8_t max_number_of_elements = 8 * sizeof(elements_t::mask_type);
using partition_count_t = unsigned long;
using table3d_t = std::array<std::array<partition_count_t, max_number_of_elements * (max_number_of_elements + 1) / 2>, max_number_of_elements>;

} // namespace vulkan::pipeline::partitions
