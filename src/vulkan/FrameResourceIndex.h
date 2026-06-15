#pragma once

#include "utils/Vector.h"
#include <string>

namespace vulkan {

struct FrameResourcesCategory { };
using FrameResourceIndex = utils::VectorIndex<FrameResourcesCategory>;

std::string to_string(FrameResourceIndex);
inline std::ostream& operator<<(std::ostream& os, FrameResourceIndex index) { return os << to_string(index); }

} // namespace vulkan
