// Anonymous namespace because probably only included from Defaults.cxx.
#include <iostream>

namespace vk_utils {

template<typename T>
struct PrintList
{
  size_t m_count;
  T const* m_list;
};

template<typename T>
PrintList<T> print_list(T* list, size_t count)
{
  return { count, list };
}

template<typename T>
std::ostream& operator<<(std::ostream& os, PrintList<T> const& list)
{
  os << '<';
  for (int i = 0; i < list.m_count; ++i)
  {
    if (i > 0)
      os << ", ";
    os << list.m_list[i];
  }
  os << '>';
  return os;
}

template<>
std::ostream& operator<<(std::ostream& os, PrintList<char const* const> const& list);

} // namespace vk_utils
