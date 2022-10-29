#pragma once

#include "utils/Vector.h"

namespace vulkan::shader_builder {

struct ShaderResourceIndexCategory;
using ShaderResourceIndex = utils::VectorIndex<ShaderResourceIndexCategory>;

} // namespace vulkan::shader_builder
