#pragma once

#include "utils/iomanip.h"
#include <boost/intrusive_ptr.hpp>
#include <memory>

namespace vk_utils {

template<typename T>
class PrintingPointer : public utils::iomanip::Sticky
{
 private:
  static utils::iomanip::Index s_index;

 public:
  PrintingPointer(long iword_value) : Sticky(s_index, iword_value) { }

  static long get_iword_value(std::ostream& os) { return get_iword_from(os, s_index); }
};

//static
template<typename T>
utils::iomanip::Index PrintingPointer<T>::s_index;

template<typename T>
struct PrintPointer
{
  T const* m_ptr;
};

template<typename T>
PrintPointer<T> print_pointer(T const* ptr)
{
  return { ptr };
}

template<typename T>
PrintPointer<T> print_pointer(std::unique_ptr<T> const& ptr)
{
  return { ptr.get() };
}

template<typename T>
PrintPointer<T> print_pointer(boost::intrusive_ptr<T> const& ptr)
{
  return { ptr.get() };
}

template<typename T>
std::ostream& operator<<(std::ostream& os, PrintPointer<T> ptr)
{
  if (ptr.m_ptr)
  {
    if (!PrintingPointer<T>::get_iword_value(os))
    {
      os << '&';
      os << PrintingPointer<T>(1L) << *ptr.m_ptr;
      os << '@';
    }
    os << (void*)ptr.m_ptr;
  }
  else
    os << "nullptr";
  return os;
}

} // namespace vk_utils
