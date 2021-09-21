#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace utils {

// Return a vector of all names listed in `required_names` that are missing in `available_names`.
std::vector<char const*> find_missing_names(
    std::vector<char const*> const& required_names,
    std::vector<char const*> const& available_names);

} // namespace utils
