#pragma once

#include "utils/Vector.h"
#include <string>

namespace vulkan {

class Swapchain;
using SwapchainIndex = utils::VectorIndex<Swapchain>;

std::string to_string(SwapchainIndex);
inline std::ostream& operator<<(std::ostream& os, SwapchainIndex index) { return os << to_string(index); }

} // namespace vulkan
