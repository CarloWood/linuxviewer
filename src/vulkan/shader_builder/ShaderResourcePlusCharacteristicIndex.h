#pragma once

#include "utils/Vector.h"

namespace vulkan::shader_builder {

struct ShaderResourcePlusCharacteristicIndexCategory;
using ShaderResourcePlusCharacteristicIndex = utils::VectorIndex<ShaderResourcePlusCharacteristicIndexCategory>;

} // namespace vulkan::shader_builder
