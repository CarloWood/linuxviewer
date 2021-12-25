#pragma once

#include <vulkan/vulkan.hpp>
#include <iosfwd>

namespace vk_utils {

#ifdef CWDEBUG
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
#else
inline void const* print_chain(void const* pNext)
{
  return pNext;
}
#endif

} // namespace vk_utils
