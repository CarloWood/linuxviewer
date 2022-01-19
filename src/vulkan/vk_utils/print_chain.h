#pragma once

#include "utils/iomanip.h"
#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vk_utils {

#ifdef CWDEBUG
class ChainManipulator : public utils::iomanip::Sticky
{
 private:
  static utils::iomanip::Index s_index;

 public:
  ChainManipulator() : Sticky(s_index, true) { }

  static long get_iword_value(std::ostream& os) { return get_iword_from(os, s_index); }
};

struct PNextChain
{
  vk::StructureType sType;
  void* pNext;
};

inline PNextChain const* print_chain(void const* pNext)
{
  return reinterpret_cast<PNextChain const*>(pNext);
}

std::ostream& operator<<(std::ostream& os, PNextChain const* chain);
#else // CWDEBUG
inline void const* print_chain(void const* pNext)
{
  return pNext;
}
#endif // CWDEBUG

} // namespace vk_utils
