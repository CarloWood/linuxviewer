#include "sys.h"
#include "check_instance_extensions_availability.h"
#include "find_missing_extensions.h"
#include "utils/print_using.h"
#include "utils/QuotedList.h"
#include "utils/AIAlert.h"
#include "debug.h"
#include <vulkan/vulkan.hpp>
#include <unordered_set>
#include <string>

namespace vulkan {

void check_instance_extensions_availability(std::vector<char const*> const& required_extensions)
{
  DoutEntering(dc::vulkan, "check_instance_extensions_availability(required_extensions:" << print_using(required_extensions, QuotedList{}) << ")");

  auto available_extensions = vk::enumerateInstanceExtensionProperties(nullptr);

#ifdef CWDEBUG
  std::vector<char const*> r;
  for (auto&& property : available_extensions)
    r.push_back(property.extensionName);
  Dout(dc::vulkan, "Available instance extensions:" << print_using(r, QuotedList{}));
#endif

  // Check that every extension name in required_extensions is available.
  auto missing_extensions = find_missing_extensions(required_extensions, available_extensions);

  if (!missing_extensions.empty())
  {
    std::string missing_extensions_str;
    std::string prefix;
    for (char const* missing : missing_extensions)
    {
      missing_extensions_str += prefix + missing;
      prefix = ", ";
    }
    THROW_ALERT("Missing required extension(s) {[MISSING_EXTENSIONS]}", AIArgs("[MISSING_EXTENSIONS]", utils::print_using(missing_extensions, utils::QuotedList{})));
  }
}

} // namespace vulkan
