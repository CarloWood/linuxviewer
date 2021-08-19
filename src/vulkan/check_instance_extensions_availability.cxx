#include "sys.h"
#include "check_instance_extensions_availability.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <vulkan/vulkan.hpp>
#include <unordered_set>
#include <string>

namespace vulkan {

void check_instance_extensions_availability(std::vector<char const*> const& required_extensions)
{
  DoutEntering(dc::vulkan|continued_cf, "check_instance_extensions_availability(required_extensions:{");
  CWDEBUG_ONLY(char const* prefix = " ");
  // Copy the names of the required extensions to a new, mutable vector.
  std::vector<char const*> missing_extensions(required_extensions.size());
  int i = 0;
  for (char const* required : required_extensions)
  {
    Dout(dc::continued, prefix << '"' << required << '"');
    CWDEBUG_ONLY(prefix = ", ");
    missing_extensions[i++] = required;
  }
  Dout(dc::finish, " })");

  auto available_instance_extensions = vk::enumerateInstanceExtensionProperties(nullptr);

  // Check that every extension name in required_extensions is available.
  Dout(dc::vulkan, "Available instance extensions:");
  {
    CWDEBUG_ONLY(debug::Mark m);
    for (auto&& extension : available_instance_extensions)
    {
      Dout(dc::vulkan, extension.extensionName);
      missing_extensions.erase(
          std::remove(missing_extensions.begin(), missing_extensions.end(), static_cast<std::string_view>(extension.extensionName)),
          missing_extensions.end());
    }
  }

  if (!missing_extensions.empty())
  {
    std::string missing_extensions_str;
    std::string prefix;
    for (char const* missing : missing_extensions)
    {
      missing_extensions_str += prefix + missing;
      prefix = ", ";
    }
    THROW_ALERT("Missing required extension(s) {[MISSING_EXTENSIONS]}", AIArgs("[MISSING_EXTENSIONS]", missing_extensions_str));
  }
}

} // namespace vulkan
