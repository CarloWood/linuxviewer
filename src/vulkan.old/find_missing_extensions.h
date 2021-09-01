#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

namespace vulkan {

// Return a vector of all extensions listed in `required_extensions` that are missing in `available_extensions`.
template<typename ExtensionPropertiesAllocator>
std::vector<char const*> find_missing_extensions(
    std::vector<char const*> const& required_extensions,
    std::vector<vk::ExtensionProperties, ExtensionPropertiesAllocator> const& available_extensions)
{
  // Copy the names of the required extensions to a new, mutable vector.
  std::vector<char const*> missing_extensions(required_extensions.size());
  int i = 0;
  for (char const* required : required_extensions)
    missing_extensions[i++] = required;

  // Remove the extension names that are in available extensions.
  for (auto&& available : available_extensions)
    missing_extensions.erase(
        std::remove(missing_extensions.begin(), missing_extensions.end(), static_cast<std::string_view>(available.extensionName)),
        missing_extensions.end());

  return missing_extensions;
}

} // namespace vulkan
