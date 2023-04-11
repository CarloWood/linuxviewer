#pragma once

#include "../shader_builder/BasicType.h"
#include <vulkan/vulkan.hpp>

namespace vk_utils {

vk::Format type2format(vulkan::shader_builder::BasicType glsl_type);

} // namespace vk_utils
