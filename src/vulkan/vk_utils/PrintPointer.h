// Anonymous namespace because probably only included from Defaults.cxx.
#include <iostream>

namespace vk_utils {

template<typename T>
struct PrintPointer
{
  T const* m_ptr;
};

template<typename T>
PrintPointer<T> print_pointer(T* ptr)
{
  return { ptr };
}

template<typename T>
std::ostream& operator<<(std::ostream& os, PrintPointer<T> ptr)
{
  if (ptr.m_ptr)
  {
    os << '&';
    os << *ptr.m_ptr;
  }
  else
    os << "nullptr";
  return os;
}

} // namespace vk_utils
