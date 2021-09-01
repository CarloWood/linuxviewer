#pragma once

#include <vector>

namespace vulkan {

void check_instance_extensions_availability(std::vector<char const*> const& required_extensions);

} // namespace vulkan
