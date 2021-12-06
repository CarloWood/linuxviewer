#include "sys.h"
#include "PrintList.h"

namespace vk_utils {

template<>
std::ostream& operator<<(std::ostream& os, PrintList<char const* const> const& list)
{
  os << '<';
  for (int i = 0; i < list.m_count; ++i)
  {
    if (i > 0)
      os << ", ";
    os << '"' << list.m_list[i] << '"';
  }
  os << '>';
  return os;
}

} // namespace vk_utils
