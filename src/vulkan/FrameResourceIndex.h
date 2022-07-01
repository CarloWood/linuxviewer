#pragma once

#include "utils/Vector.h"
#include <string>

namespace vulkan {

struct FrameResourcesData;
using FrameResourceIndex = utils::VectorIndex<FrameResourcesData>;

std::string to_string(FrameResourceIndex);
inline std::ostream& operator<<(std::ostream& os, FrameResourceIndex index) { return os << to_string(index); }

} // namespace vulkan
