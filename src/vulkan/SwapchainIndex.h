#pragma once

#include "utils/Vector.h"
#include <string>

namespace vulkan {

struct SwapchainCategory { };
using SwapchainIndex = utils::VectorIndex<SwapchainCategory>;

std::string to_string(SwapchainIndex);
inline std::ostream& operator<<(std::ostream& os, SwapchainIndex index) { return os << to_string(index); }

} // namespace vulkan
