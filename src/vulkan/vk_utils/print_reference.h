#pragma once

#include "utils/iomanip.h"

namespace vk_utils {

template<typename T>
class PrintingReference : public utils::iomanip::Sticky
{
 private:
  static utils::iomanip::Index s_index;

 public:
  PrintingReference(long iword_value) : Sticky(s_index, iword_value) { }

  static long get_iword_value(std::ostream& os) { return get_iword_from(os, s_index); }
};

//static
template<typename T>
utils::iomanip::Index PrintingReference<T>::s_index;

template<typename T>
struct PrintReference
{
  T const& m_ref;
};

template<typename T>
PrintReference<T> print_reference(T const& ref)
{
  return { ref };
}

template<typename T>
std::ostream& operator<<(std::ostream& os, PrintReference<T> ref)
{
  os << '@' << (void*)&ref.m_ref;
  if (!PrintingReference<T>::get_iword_value(os))
    os << ':' << PrintingReference<T>(1L) << ref.m_ref;
  return os;
}

} // namespace vk_utils
