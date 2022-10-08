#include "sys.h"
#include "snake_case.h"
#include <cctype>

namespace vk_utils {

std::string snake_case(std::string_view camel_case)
{
  std::string result;
  char const* prefix = "";
  for (char c : camel_case)
  {
    if (std::isupper(c))
    {
      result += prefix;
      result += std::tolower(c);
      prefix = "_";
      continue;
    }
    result += c;
  }
  return result;
}

} // namespace vulkan::vk_utils
