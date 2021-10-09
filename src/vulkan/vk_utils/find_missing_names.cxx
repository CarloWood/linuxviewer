#include "sys.h"
#include "find_missing_names.h"
#include <algorithm>

namespace vk_utils {

// Return a vector of all names listed in `required_names` that are missing in `available_names`.
std::vector<char const*> find_missing_names(
    std::vector<char const*> const& required_names,
    std::vector<char const*> const& available_names)
{
  // Copy the names of the required names to a new, mutable vector.
  std::vector<char const*> missing_names(required_names);

  // Remove the names that are in available_names.
  for (auto&& available : available_names)
    missing_names.erase(
        std::remove_if(missing_names.begin(), missing_names.end(), [available](char const* name){ return strcmp(available, name) == 0; }),
        missing_names.end());

  // Return a vector with names that are in required_names but not in available_names.
  return missing_names;
}

} // namespace vk_utils
