#include "sys.h"
#include "SwapchainIndex.h"

namespace vulkan {

std::string to_string(SwapchainIndex index)
{
  std::string result = "s";
  result += std::to_string(index.get_value());
  return result;
}

} // namespace vulkan
