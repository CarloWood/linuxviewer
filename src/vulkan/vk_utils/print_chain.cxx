#include "sys.h"
#ifdef CWDEBUG
#include "print_chain.h"
#include "debug/debug_ostream_operators.h"
#endif

namespace vk_utils {

#ifdef CWDEBUG

//static
utils::iomanip::Index ChainManipulator::s_index;

std::ostream& operator<<(std::ostream& os, PNextChain const* chain)
{
  ChainManipulator setchain;

  switch (chain->sType)
  {
    case vk::StructureType::ePhysicalDeviceVulkan11Features:
    {
      vk::PhysicalDeviceVulkan11Features const* feature = reinterpret_cast<vk::PhysicalDeviceVulkan11Features const*>(chain);
      os << setchain << " ⛓ " << *feature;
      break;
    }
    case vk::StructureType::ePhysicalDeviceVulkan12Features:
    {
      vk::PhysicalDeviceVulkan12Features const* feature = reinterpret_cast<vk::PhysicalDeviceVulkan12Features const*>(chain);
      os << setchain << " ⛓ " << *feature;
      break;
    }
    default:
      os << to_string(chain->sType);
  }

  if (chain->pNext)
    os << print_chain(chain->pNext);

  return os;
}

#endif // CWDEBUG

} // namespace vk_utils
