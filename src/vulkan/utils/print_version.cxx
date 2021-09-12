#include "sys.h"
#include "print_version.h"
#include <vulkan/vulkan_core.h>

namespace vk_utils {

std::string print_version(uint32_t version)
{
  std::string version_str = std::to_string(VK_API_VERSION_MAJOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_MINOR(version));
  version_str += '.';
  version_str += std::to_string(VK_API_VERSION_PATCH(version));
  return version_str;
}

} // namespace vk_utils
